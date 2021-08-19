#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
typedef struct
{
    int valid;
    int marked;
    int time;
} cache;
int hexTodec(char *hex);
int flag = 0;
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
int main(int argc, char *argv[])
{
    const char *usage = "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>"
                        "• -h: Optional help flag that prints usage info"
                        "• -v: Optional verbose flag that displays trace info"
                        "• -s <s>: Number of set index bits (S = 2^s is the number of sets)"
                        "• -E <E>: Associativity (number of lines per set)"
                        "• -b <b>: Number of block bits (B = 2^b is the block size)"
                        "• -t <tracefile>: Name of the valgrind trace to replay";
    int s, E, b;
    int opt;
    char *fileloc;
    cache *set; //代表高速缓存组
    while ((opt = getopt(argc, argv, "hvsEbt")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf(usage);
            break;
        case 'v':
            flag = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            fileloc = optarg;
            break;
        default:
            fprintf(stderr, "Usage error");
            exit(EXIT_FAILURE);
        }
    }
    int S = pow(2, s);
    FILE *file = NULL;
    file = fopen(fileloc, "r");
    set = (cache *)malloc(sizeof(cache) * S * E);
    char *temp = (char *)malloc(20);
    int time = 0;
    //比如地址为7fefe059c ->十进制
    while (fgets(temp, 20, file) != NULL)
    {
        if (temp[0] == 'I')
            continue;
        if (flag)
        {
            printf(temp);
        }
        char mode = temp[1];
        char *addbegin = temp[3];
        int address = hexTodec(addbegin);
        switch (mode)
        {
        case 'L':
            load(address, b, s, set, E, time);
            printf("\n");
            break;
        case 'S':
            load(address, b, s, set, E, time);
            printf("\n");
            break;
        case 'M':
            load(address, b, s, set, E, time);
            load(address, b, s, set, E, time);
            printf("\n");
            break;
        }
        time++;
    }
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
int hexTodec(char *hex)

{
    int decimal = 0;
    char *temp = hex;
    while (*temp != ',')
    {
        char num = *temp;
        switch (num)
        {
        case 'A':
        case 'a':
            decimal = decimal * 10 + 10;
            break;
        case 'B':
        case 'b':
            decimal = decimal * 10 + 11;
            break;
        case 'C':
        case 'c':
            decimal = decimal * 10 + 12;
            break;
        case 'D':
        case 'd':
            decimal = decimal * 10 + 13;
            break;
        case 'E':
        case 'e':
            decimal = decimal * 10 + 14;
            break;
        case 'F':
        case 'f':
            decimal = decimal * 10 + 15;
            break;
        }
        temp++;
    }
}

void load(int address, int b, int s, const cache *set, int E, int time)
{
    for (int i = 0; i < b; i++)
    {
        address /= 2;
    }
    int gindex = 0;
    for (int i = 0; i < s; i++)
    {
        gindex = gindex * 2 + address % 2;
        address /= 2;
    }
    int t = 0;
    while (address)
    {
        t = t * 2 + address % 2;
        address /= 2;
    }
    //gindex = 3;那就从3*E 开始找起,4*E
    cache *search;
    int hasEmpty = 0;
    cache *eviction_cache;
    int minTime = 1000000000;
    for (search = set + gindex * E; search < set + (gindex + 1) * E; search++)
    {
        if (!(*search).valid)
        {
            hasEmpty = 1;
            eviction_cache = search;
        }
        else
        {
            if (!hasEmpty && (*search).time < minTime)
            {
                minTime = (*search).time;
                eviction_cache = search;
            }
        }
        if ((*search).valid && (*search).marked == t)
        {
            if (flag)
            {
                printf(" hit");
                hit_count++;
                return;
            }
            else
            {
                hit_count++;
                return;
            }
        }
    }
    miss_count++;
    if (flag)
        printf(" miss");
    if (!hasEmpty)
    {
        eviction_count++;
        if (flag)
            printf(" eviction");
        eviction_cache->valid = 1;
        eviction_cache->time = time;
        eviction_cache->marked = t;
    }
    else
    {
        eviction_cache->valid = 1;
        eviction_cache->time = time;
        eviction_cache->marked = t;
    }
}
