# CacheLab

任务分解：
1.程序参数的输入
getopt function  
man 3 getopt for details

    #include<getopt.h>
    #include <stdlib.h>
    #include <unistd.h>

具体的参数

    Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
    • -h: Optional help flag that prints usage info
    • -v: Optional verbose flag that displays trace info
    • -s <s>: Number of set index bits (S = 2^s is the number of sets)
    • -E <E>: Associativity (number of lines per set)
    • -b <b>: Number of block bits (B = 2^b is the block size)
    • -t <tracefile>: Name of the valgrind trace to replay
    
    e.g : ./csim-ref -v -s 4 -E 1 -b 4 -t traces/yi.trace

-h:打印上面的字符串
-v:打印每次操作的详细信息

    L 10,1 miss
    M 20,1 miss hit
    L 22,1 hit
    S 18,1 hit
    L 110,1 miss eviction
    L 210,1 miss eviction
    M 12,1 miss eviction hit
    hits:4 misses:5 evictions:3
-s: S = 2^s
-E: 每个组的行数
-b: B = 2^b 块大小
-t: 文件的位置
**这里B似乎不用表示出来，S ， L等操作也只是象征性的操作...**

2.创建cache 
可以使用一个结构来表示cache
cache中应该有下列参数：
组号，有效位，标记

3.对L，S，M的具体分析
L是load操作
S这里也可以理解为Load操作
M是修改，先进行一个LOAD再进行一次Store操作，相当于两次load

getopt用法

    while ((opt = getopt(argc, argv, "nt:")) != -1) {
                switch (opt) {
                case 'n':
                    flags = 1;
                    break;
                case 't':
                    nsecs = atoi(optarg);
                    tfnd = 1;
                    break;
                default: /* '?' */
                    fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                            argv[0]);
                    exit(EXIT_FAILURE);
                }
            }

test
    