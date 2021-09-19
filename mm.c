// 分配器输出三个函数到应用程序中
// extern int mm_init();
// extern void *mm_malloc(size_t size);
// extern void mm_free(void *ptr);
// mm_init 成功返回0 ， 失败返回-1
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE 1 << 12                    //extend heap by this amount
#define MAX(x, y) ((x) > (y)) ? (x) : (y)    //这里都要加括号防止有歧义
#define PACK(size, alloc) ((size) | (alloc)) //打包大小和分配位给footer or header
//在一个地址p上，读写一个字
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
//在一个地址p上，读取大小和分配位
#define GET_SIZE(p) (*(unsigned int *)(p) & ~0x7)
#define GET_ALLOC(p) (*(unsigned int *)(p)&0x1)
//bp 指向一个空闲块/分配块的开始位置
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
extern int mm_init();
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);
static char *heap_listp;
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void)*-1)
    {
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(1, DSIZE));
    PUT(heap_listp + 2 * WSIZE, PACK(1, DSIZE));
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));
    heap_listp += 2 * WSIZE;
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
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
    PUT(HDRP(NEXT_BLKP(bp)), pack(0, 1))
    return coalesce(bp);
}
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc & next_alloc)
    {
        return bp;
    }
    if (prev_alloc & !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    if (!prev_alloc & next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    if (!prev_alloc & !next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t entendsize;
    char *bp;
    if (size == 0)
    {
        return NULL;
    }
    if (size < DSIZE)
    {
        asize = DSIZE;
    }
    else
    {
        asize = DSIZE * (size + DSIZE + DSIZE - 1) / DSIZE;
    }
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(CHUNKSIZE, asize);
    if ((bp = extend_heap(entendsize / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, asize);
    return bp;
}
static void *find_fit(size_t asize)
{
    char *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)); bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp)))
        {
            return bp;
        }
    }
    return NULL;
}
static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    if (size - asize >= 2 * DSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size - asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size - asize, 0));
        bp = NEXT_BLKP(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}
