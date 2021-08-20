#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct
{
    int valid;
    int marked;
    int time;
} cache;
int hexTodec(char *hex);
void load(unsigned int address, unsigned int b, unsigned s, cache *set, unsigned E, int time);
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
    while ((opt = getopt(argc, argv, "hvs:E:-b:-t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("%s", usage);
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
    for (cache *node = set; node < set + S * E; node++)
    {
        node->marked = 0;
        node->time = 0;
        node->valid = 0;
    }
    char *temp = (char *)malloc(20);
    int time = 0;
    //比如地址为7fefe059c ->十进制
    while (fgets(temp, 20, file) != NULL)
    {
        if (temp[0] == 'I')
            continue;
        if (flag)
        {
            int i = 1;
            while (temp[i] != '\n')
            {
                putchar(temp[i]);
                i++;
            }
        }
        char mode = temp[1];
        char *addbegin = temp + 3;
        int address = hexTodec(addbegin);
        switch (mode)
        {
        case 'L':
            load(address, b, s, set, E, time);
            if (flag)
                printf("\n");
            break;
        case 'S':
            load(address, b, s, set, E, time);
            if (flag)
                printf("\n");
            break;
        case 'M':
            load(address, b, s, set, E, time);
            load(address, b, s, set, E, time);
            if (flag)
                printf("\n");
            break;
        }
        time++;
    }
    fclose(file);
    free(set);
    free(temp);
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
            decimal = decimal * 16 + 10;
            break;
        case 'B':
        case 'b':
            decimal = decimal * 16 + 11;
            break;
        case 'C':
        case 'c':
            decimal = decimal * 16 + 12;
            break;
        case 'D':
        case 'd':
            decimal = decimal * 16 + 13;
            break;
        case 'E':
        case 'e':
            decimal = decimal * 16 + 14;
            break;
        case 'F':
        case 'f':
            decimal = decimal * 16 + 15;
            break;
        default:
            decimal = decimal * 16 + num - '0';
        }

        temp++;
    }
    return decimal;
}

void load(unsigned int address, unsigned int b, unsigned s, cache *set, unsigned E, int time)
{
    for (int i = 0; i < b; i++)
    {
        address>>=1;
    }
    unsigned int gindex = 0;
    for (int i = 0; i < s; i++)
    {
        gindex = gindex * 2 + address % 2;
        address>>=1;
    }
    unsigned  t = address;

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
                search->time = time;
                hit_count++;
                return;
            }
            else
            {
                hit_count++;
                search->time = time;
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
