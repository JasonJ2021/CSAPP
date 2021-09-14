# CacheLab

## PartA Cache Simulator
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


    

## PartB Matrix Transpose
目标是写出尽可能降低不命中率的倒置函数

    char transpose_submit_desc[] = "Transpose submission";
    void transpose_submit(int M, int N, int A[N][M], int B[M][N]);

### 要求：
- 编译没有警告
- 最多可以定义12个int 变量在每个倒置函数中
- 不能用迭代
- 如果用了helper functions ,helper functions 和 transpose func 加起来的变量不能超过12个
- 不能修改数组A

### Evaluation For PartB

    • 32 × 32 (M = 32, N = 32)
    • 64 × 64 (M = 64, N = 64)
    • 61 × 67 (M = 61, N = 67)
    • 32 × 32: 8 points if m < 300, 0 points if m > 600
    • 64 × 64: 8 points if m < 1, 300, 0 points if m > 2, 000
    • 61 × 67: 10 points if m < 2, 000, 0 points if m > 3, 000

使用的cache规模是(s = 5, E = 1, b = 5).

    linux> make
    linux> ./test-trans -M 32 -N 32
### Working on PartB

可以自己写多种版本的transfunctions,每个的格式如下：

    char trans_simple_desc[] = "A simple transpose";
    void trans_simple(int M, int N, int A[N][M], int B[M][N])
    {
    /* your transpose code here */
    }
    --------------------------------------------------------------
    registerTransFunction(trans_simple, trans_simple_desc);

改进建议：
1.因为是直接映射cache，所以冲突不命中是一个主要的问题
2.分块是一个好办法
3.可以使用./csim-ref -v -s 5 -E 1 -b 5 -t trace.f0 来查看具体的情况

对角线上会多一次Miss

    L 10d080,4 miss eviction //会导致一个miss
    L 10d084,4 hit 
    L 10d088,4 hit 
    L 10d08c,4 hit 
    L 10d090,4 hit 
    L 10d094,4 hit 
    L 10d098,4 hit 
    L 10d09c,4 hit 
    S 14d080,4 miss eviction //第一个对角线元素冲突
    S 14d100,4 miss 
    S 14d180,4 miss 
    S 14d200,4 miss 
    S 14d280,4 miss 
    S 14d300,4 miss 
    S 14d380,4 miss 
    S 14d400,4 miss 
    L 10d100,4 miss eviction 
    L 10d104,4 hit 
    L 10d108,4 hit 
    L 10d10c,4 hit 
    L 10d110,4 hit 
    L 10d114,4 hit 
    L 10d118,4 hit 
    L 10d11c,4 hit 
    S 14d084,4 hit 
    S 14d104,4 miss eviction //第二个对角线元素冲突