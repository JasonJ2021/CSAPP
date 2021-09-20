/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MINBLOCKSIZE 16

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc)) /* Pack a size and allocated bit into a word */

#define GET(p) (*(unsigned int *)(p))              /* read a word at address p */
#define PUT(p, val) (*(unsigned int *)(p) = (val)) /* write a word at address p */

#define GET_SIZE(p) (GET(p) & ~0x7) /* read the size field from address p */
#define GET_ALLOC(p) (GET(p) & 0x1) /* read the alloc field from address p */

#define HDRP(bp) ((char *)(bp)-WSIZE)                        /* given block ptr bp, compute address of its header */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) /* given block ptr bp, compute address of its footer */

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))         /* given block ptr bp, compute address of next blocks */
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE)) /* given block ptr bp, compute address of prev blocks */
#define GET_PREV(bp) (*(unsigned int *)(bp))
#define GET_NEXT(bp) (*((unsigned int *)(bp) + 1))
#define SET_PREV(bp, val) (*(unsigned int *)(bp) = (val))
#define SET_NEXT(bp, val) (*((unsigned int *)(bp)+1) = (val))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
void *mm_malloc(size_t size);
void mm_free(void *ptr);
int mm_init(void);
void *mm_realloc(void *ptr, size_t size);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *next_fit(size_t asize);
static void place(void *bp, size_t asize);
static char *prev_listp; //记录上一个block的位置
static void *best_fit(size_t asize);
static char *list_start = NULL;
static char *heap_listp = NULL;
/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(12 * WSIZE)) == (void *)-1)
    {
        return -1;
    }
    PUT(heap_listp, 0);             //4~31
    PUT(heap_listp + 1 * WSIZE, 0); //32~63
    PUT(heap_listp + 2 * WSIZE, 0); //64~127
    PUT(heap_listp + 3 * WSIZE, 0); //128~255
    PUT(heap_listp + 4 * WSIZE, 0); //256~511
    PUT(heap_listp + 5 * WSIZE, 0); //512~1023
    PUT(heap_listp + 6 * WSIZE, 0); //1024~2047
    PUT(heap_listp + 7 * WSIZE, 0); //2048~4095
    PUT(heap_listp + 8 * WSIZE, 0); //4096~infinite
    PUT(heap_listp + 9 * WSIZE, PACK(1, DSIZE));
    PUT(heap_listp + 10 * WSIZE, PACK(1, DSIZE));
    PUT(heap_listp + 11 * WSIZE, PACK(0, 1));
    list_start = heap_listp;
    heap_listp += 10 * WSIZE;
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
}
static int getListID(size_t size)
{
    if (size <= 32)
    {
        return 0;
    }
    else if (size <= 64)
    {
        return 1;
    }
    else if (size <= 128)
    {
        return 2;
    }
    else if (size <= 256)
    {
        return 3;
    }
    else if (size <= 512)
    {
        return 4;
    }
    else if (size <= 1024)
    {
        return 5;
    }
    else if (size <= 2048)
    {
        return 6;
    }
    else if (size <= 4096)
    {
        return 7;
    }
    else
    {
        return 8;
    }
}
static void insert_list(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    int id = getListID(GET_SIZE(HDRP(ptr)));
    void *bp = list_start + id * WSIZE;
    void *prev = bp;
    void *next = GET(bp);
    while (next != NULL)
    {
        if (GET_SIZE(HDRP(next)) >= GET_SIZE(HDRP(ptr)))
            break;
        prev = next;
        next = GET_NEXT(next);
    }
    //因为根节点只有4字节，所以需要特殊处理
    if (prev == bp)
    {
        PUT(prev, ptr);
        SET_PREV(ptr, NULL);
        SET_NEXT(ptr, next);
        if (next != NULL)
            SET_PREV(next, ptr);
    }
    else
    {
        SET_NEXT(prev, ptr);
        SET_PREV(ptr, prev);
        SET_NEXT(ptr, next);
        if (next != NULL)
            SET_PREV(next, ptr);
    }
}
static void remove_list(void *ptr)
{
    if (ptr == NULL || GET_ALLOC(ptr))
        return;
    int id = getListID(GET_SIZE(HDRP(ptr)));
    void *root = list_start + id * WSIZE;
    void *prev = GET_PREV(ptr);
    void *next = GET_NEXT(ptr);
    if (prev == NULL)
    {
        PUT(root, next);
        if (next != NULL)
            SET_PREV(next, NULL);
    }
    else
    {
        SET_NEXT(prev, next);
        if (next != NULL)
            SET_PREV(next, prev);
    }
    SET_PREV(ptr, NULL);
    SET_NEXT(ptr, NULL);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    if (size == 0)
    {
        return NULL;
    }
    if (size < DSIZE)
    {
        asize = 2 * DSIZE;
    }
    else
    {
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    }
    if ((bp = first_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(CHUNKSIZE, asize);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    //从分配块转换到空闲块需要把PREV NEXT置零
    SET_NEXT(ptr, 0);
    SET_PREV(ptr, 0);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    size = GET_SIZE(HDRP(oldptr));
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize - WSIZE);
    mm_free(oldptr);
    return newptr;
}
static void *extend_heap(size_t words)
{
    size_t size;
    char *bp;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    SET_NEXT(bp, 0);
    SET_PREV(bp, 0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc)
    {
        insert_list(bp);
    }
    if (prev_alloc && !next_alloc)
    {
        remove_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_list(bp);
    }
    if (!prev_alloc && next_alloc)
    {
        remove_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_list(bp);
    }
    if (!prev_alloc && !next_alloc)
    {
        remove_list(PREV_BLKP(bp));
        remove_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_list(bp);
    }
    return bp;
}
static void *first_fit(size_t asize)
{
    int id = getListID(asize);
    void *bp;
    while (id <= 8)
    {
        for (bp = GET(list_start + id*WSIZE); bp != NULL; bp = GET_NEXT(bp))
        {
            if (GET_SIZE(HDRP(bp)) >= asize)
            {
                return bp;
            }
        }
        id++;
    }
    return NULL;
}

static void *next_fit(size_t asize)
{
    char *bp;
    for (bp = prev_listp; GET_SIZE(HDRP(bp)); bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp)))
        {
            prev_listp = bp;
            return bp;
        }
    }
    for (bp = heap_listp; bp != prev_listp; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp)))
        {
            prev_listp = bp;
            return bp;
        }
    }
    return NULL;
}
static void *best_fit(size_t asize)
{
    char *bp;
    char *final = NULL;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)); bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp)))
        {
            if (final == NULL)
            {
                final = bp;
            }
            else
            {
                if (GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(final)))
                {
                    final = bp;
                }
            }
        }
    }
    return final;
}
static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    if (size - asize >= 2 * DSIZE)
    {
        remove_list(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *new_bp = NEXT_BLKP(bp); //不能修改原来的bp
        PUT(HDRP(new_bp), PACK(size - asize, 0));
        PUT(FTRP(new_bp), PACK(size - asize, 0));
        SET_NEXT(new_bp, 0);
        SET_PREV(new_bp, 0);
        insert_list(new_bp);
    }
    else
    {
        remove_list(bp);
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}
