- [12 Concurrent Programming](#12-concurrent-programming)
  - [12.2  基于I/O多路复用的并发编程](#122--基于io多路复用的并发编程)
    - [12.2.1 基于I/O多路复用的并发事件驱动服务器](#1221-基于io多路复用的并发事件驱动服务器)
      - [pool定义](#pool定义)
      - [main主函数](#main主函数)
      - [初始化pool](#初始化pool)
      - [添加客户端连接](#添加客户端连接)
      - [检查所有客户端中echo请求](#检查所有客户端中echo请求)
  - [12.3 基于线程的并发编程](#123-基于线程的并发编程)
    - [12.3.1 线程执行模型](#1231-线程执行模型)
    - [12.3.2 Posix线程](#1232-posix线程)
    - [12.3.3 创建线程](#1233-创建线程)
    - [12.3.4 终止进程](#1234-终止进程)
    - [12.3.5 回收终止进程的资源](#1235-回收终止进程的资源)
    - [12.3.6 分离线程](#1236-分离线程)
    - [12.3.7 初始化线程](#1237-初始化线程)
    - [12.3.8 基于线程的并发服务器](#1238-基于线程的并发服务器)
  - [12.4 多线程程序中的共享变量](#124-多线程程序中的共享变量)
    - [12.4.1 线程内存模型](#1241-线程内存模型)
  - [12.5 用信号量同步线程](#125-用信号量同步线程)
    - [12.5.2 信号量](#1252-信号量)
    - [12.5.3 使用信号量来实现互斥](#1253-使用信号量来实现互斥)
    - [12.5.4 利用信号量来调度共享资源](#1254-利用信号量来调度共享资源)
      - [1. 生产者-消费者问题](#1-生产者-消费者问题)
      - [2. 读者-写者问题](#2-读者-写者问题)
        - [第一类读者写者问题：](#第一类读者写者问题)
    - [12.5.5 基于预线程化的并发服务器](#1255-基于预线程化的并发服务器)
  - [12.6 使用线程提高并行性](#126-使用线程提高并行性)
## 12 Concurrent Programming
### 12.2  基于I/O多路复用的并发编程
现在我们echo服务器要求能够对用户从stdin输入的交互命令作出相应，因此这时候服务器要响应两个IO事件
- 连接请求
- 键入命令行

一个解决办法就是I/O multiplexing技术，使用select函数，挂起进程，只有在一个或者多个I/O发生之后才返回给应用程序
例如：
- 当集合{0,4}中任意描述符准备好读时返回
- 当集合{1,2,7}中任意描述符准备好写时返回
- 在等待一个IO时间发生过了152.13秒超时

      #include <sys/select.h>
      int select(int n , fd_set *fdset,NULL,NULL,NULL);

      FD_ZERO(fd_set *fdset); //把fdset中所有bits 清零
      FD_CLR(int fd,fd_set *fdset);   //clear bit fd in fd set
      FD_SET(int fd , fd_set *fdset); //turn on bit fd in fdset
      FD_ISSET(int fd , fd_set *fdset);//is bit fd in fdset on ?
select函数处理描述符集合。描述符集合是一个大小为n的位向量:
bn-1,---,b1,b0
bk = 1 代表描述符k是fdset的一个元素
select函数有两个输入：读集合的描述符集合，和读集合的基数n。
select函数会一直阻塞，直到读集合至少有一个描述符准备好可以读。
select函数有一个副作用，它会改变fdset指向一个准备好集合，并且返回准备好集合的基数.因此每次调用select时都要更新读集合
    #include "csapp.h"
    void echo(int connfd);
    void command(void);

    int main(int argc , char *argv[]){
        int connfd , listenfd ;    #include "csapp.h"
    void echo(int connfd);
    void command(void);

    int main(int argc , char *argv[]){
        int connfd , listenfd ;
        socklen_t clientlen;
        struct sockaddr_storage clientaddr;
        fd_set read_set , ready_set;
        listenfd = Open_listenfd(argv[1]);
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO,&read_set);
        FD_SET(listenfd , &read_set);
        while(1){
            ready_set = read_set;
            Select(listenfd+ 1 , &ready_set,NULL,NULL,NULL);
            if(FD_ISSET(STDIN_FILENO , &ready_set)){
                command();
            }
            if(FD_ISSET(listenfd , &ready_set)){
                clientlen = sizeof(struct sockaddr_storage);
                connfd = Accept(listenfd , (SA *) &clientaddr ,&clientlen);
                echo(connfd);
                Close(connfd);
            }
        }
    }
    void command(){
        char *buf[MAXLINE];
        while(!Fgets(buf,MAXLINE,stdin)){
            exit(0);
        }
        printf("%s",buf);
    }

#### 12.2.1 基于I/O多路复用的并发事件驱动服务器
在事件驱动程序中，某些事件会导致流向前推进。一般会把逻辑流转化为状态机。
state machine : state 、input event 、transition.
转移：把状态和输入事件映射到状态 —— （输入状态 ， 输入时间） 映射到一个输出状态
##### pool定义
    typedef struct
    {
        int maxfd;
        fd_set read_set;
        fd_set ready_set;
        int nready;
        int maxi;
        int clientfd[FD_SETSIZE];
        rio_t clientrio[FD_SETSIZE];
    } pool;
定义了一个事件pool
maxfd：最大的fd
read_set:所有活跃fd的集合
ready_set:准备好读的fd集合
nready:准备好读fd的个数
maxi: clientfd数组中索引的最大值
##### main主函数
    int main(int argc, char *argv[])
    {
        int listenfd, connfd;
        socklen_t clientlen;
        struct sockaddr_storage clientaddr;
        static pool pool;
        if (argc != 2)
        {
            fprintf(stderr, "usage : %s <port> \n", argv[0]);
            exit(0);
        }
        listenfd = Open_listenfd(argv[1]);
        init_pool(listenfd, &pool);
        while (1)
        {
            pool.ready_set = pool.read_set; // 由于select的副作用，每次都要重新指向;
            pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);
            if (FD_ISSET(listenfd, &pool.ready_set))
            {
                clientlen = sizeof(struct sockaddr_storage);
                connfd = Accept(clientlen, (SA *)&clientaddr, &clientlen);
                add_client(connfd, &pool);
            }
            check_clients(&pool);
        }
    }
服务器逻辑：
- 创建事件pool,开启监听，初始化pool
- 开始循环,如果listenfd准备好读，则添加一个客户端连接
- 检查事件池中是否有echo事件，体现事件驱动性质
  
##### 初始化pool
    void init_pool(int listenfd, pool *p)
    {
        p->maxi = -1;
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            p->clientfd[i] = -1;
        }
        p->maxfd = listenfd;
        FD_ZERO(&p->read_set);
        FD_SET(listenfd, &p->read_set);
    }
开始要把listenfd加入到read_set中去.
##### 添加客户端连接
    void add_client(int connfd, pool *p)
    {
        p->nready--;
        int i;
        for (i = 0; i < FD_SETSIZE; i++)
        {
            if (p->clientfd[i] < 0)
            {
                p->clientfd[i] = connfd;
                Rio_readinitb(&p->clientrio[i], connfd);
                FD_SET(connfd, &p->read_set);
                if (connfd > p->maxfd)
                {
                    p->maxfd = connfd;
                }
                if (i > p->maxi)
                {
                    p->maxi = i;
                }
                break;
            }
        }
        if (i == FD_SETSIZE)
        {
            app_error("add_client error: Too many clients");
        }
    }
##### 检查所有客户端中echo请求
    void check_clients(pool *p)
    {
        int i, connfd, n;
        char buf[MAXLINE];
        rio_t rio;
        for (i = 0; (i <= p->maxi && (p->nready > 0)); i++)
        {
            connfd = p->clientfd[i];
            rio = p->clientrio[i];
            if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
            {
                p->nready--;
                if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
                {
                    byte_cnt += n;
                    printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                    Rio_writen(connfd, buf, n);
                }
                else
                {
                    Close(connfd);
                    FD_CLR(connfd, &p->read_set);
                    p->clientfd[connfd] = -1;
                }
            }
        }
    }

### 12.3 基于线程的并发编程
线程是运行在进程上下文中的逻辑流.它由内核自动调度，每个线程有它自己的thread context,包括TID,stack ,stack pointer,PC,寄存器。所有运行在一个进程的线程共享该进程的虚拟内存
#### 12.3.1 线程执行模型
每个进程开始时都是单一进程，称为主线程，某一时刻主线程会创建一个对等线程(peer thread)
和一个进程相关的线程组成一个对等线程池

#### 12.3.2 Posix线程
Posix线程(Pthreads)是C程序处理线程的一个standard interface.
    #include "csapp.h"
    void *thread(void *vargp);

    int main(){
        pthread_t tid;
        Pthread_create(&tid,NULL,thread,NULL);
        Pthread_join(tid,NULL);
        exit(0);
    }

    void *thread(void *vargp){
        printf("Hello, world \n");
        return NULL;
    }
线程的代码和本地数据都被封装为一个thread routine(线程例程)
> void *thread(void *vargp)

每个线程例程以一个通用指针作为输入，返回一个通用指针，如果想传递多个参数给线程例程，可以把参数放到一个结构里面。输出也是同理
Pthread_create创建一个新的对等例程，函数返回的时候，主线程和对等例程同时运行。
pthread_join调用让主线程等待对等线程终止。

#### 12.3.3 创建线程
    #include <pthread.h>
    typedef void *(func)(void *);
    int pthread_create(pthread_t *tid,pthread_attr_t *attr,func *f , void *arg);
pthread_create函数创建一个新的线程，并且带着一个输入变量arg，在新线程上下文中执行f.attr我们这里设置为NULL
tid包含着新线程的tid,而对于新线程来说可以使用pthread_self(void)来获取自己的tid

#### 12.3.4 终止进程
- 当顶层的线程例程返回时，线程隐式终止
- 通过调用pthread_exit函数。如果主线程调用，那么会等待所有其他对等线程终止，然后终止主线程和进程
>  void pthread_exit(void *thread_return);
- 某个线程调用exit(),会终止进程和所有线程
- 另一个对等线程通过当前TID参数调用pthread_cancel
> int pthread_cancel(pthread_t tid);

#### 12.3.5 回收终止进程的资源
> int pthread_join(pthread_t tid , void **thread_return);

pthread_join会阻塞，直到tid终止，将返回的void *指针赋值给thread_return.最后回收被终止进程的所有内存资源
- 注意pthread_join只能回收一个线程

#### 12.3.6 分离线程
线程是可结合的（joinable) or 分离的(detached).
可结合：能够被其他线程回收或者杀死
分离的：不能被其他线程回收或者杀死，内存资源在系统终止后被自动回收
> int pthread_detach(pthread_t tid);

pthread_detach(pthread_self)可以分离自己
在一个高性能Web服务器上，可能每次受到Web浏览器的连接请求都创建一个新的对等例程。对于服务器来说没有必要显式等待回收线程，因此在对等例程处理请求之前应该分离自身。

#### 12.3.7 初始化线程
    pthread_once_t once_control = PTHREAD_ONCE_INIT;
    int pthread_once(pthread_once_t *once_control , 
                    void (*init_toutine)(void));
once_control是一个全局或者static variable, 总是被初始化为PHTREAD——ONCE——INIT
第一个用once_control 调用thread_once时候会调用init_routine。接下来的pthread_once调用不做任何事情

#### 12.3.8 基于线程的并发服务器
    #include "csapp.h"

    void echo(int connfd);
    void *thread(void *vargp);
    int main(int argc , char *argv[]){
        int listenfd , *connfd;
        pthread_t tid;
        struct sockaddr_storage client_addr;
        int clientlen;
        if(argc != 2){
            fprintf(stderr , "usage : %s <port> \n" , argv[0]);
        }
        listenfd = Open_listenfd(argv[1]);
        while(1){
            connfd = Malloc(sizeof(int));
            clientlen = sizeof(struct sockaddr_storage);
            *connfd = Accept(listenfd,(SA * )&client_addr , &clientlen);
            Pthread_create(&tid,NULL,thread,connfd);
        }
    }
    void *thread(void *vargp){
        pthread_detach(pthread_self());
        int connfd = *((int *)vargp);
        Free(vargp);
        echo(connfd);
        Close(connfd);
        return NULL;
    }
为了避免竞争，为每个线程例程分配一个内存块,

### 12.4 多线程程序中的共享变量
#### 12.4.1 线程内存模型
每个线程都有独立的线程上下文，包括线程ID，栈，栈指针，程序计数器，条件码和通用目的寄存器。每个线程和其他线程共享进程上下文的其他部分，包括代码、读写数据、堆、所有共享库代码和数据区域。
线程也共享相同的打开文件集合
不同的线程栈被保存在虚拟地址空间的栈区域中，且不对其他线程设防

    #include "csapp.h"
    #define N 2
    void *thread(void *vargp);
    char **ptr;
    int main(){
        int i;
        pthread_t tid;
        char *msgs[N] = {
            "Hello from foo",
            "Hello from bar"
        };
        ptr = msgs;
        for(i = 0 ; i < N ; i++){
            pthread_create(&tid,NULL,thread,(void *)i);
        }
        pthread_exit(NULL);
    }
    void *thread(void *vargp){
        int myid = (int )vargp;
        static int cnt = 0;
        printf("[%d] : %s (cnt = %d)\n",myid,ptr[myid],++cnt);
        return NULL;
    }

### 12.5 用信号量同步线程
共享变量十分方便，但是也引入了同步错误(synchronization error)的可能性
    #include "csapp.h"
    void *thread(void *vargp);
    volatile long cnt = 0;

    int main(int argc , char *argv[]){
        long niters;
        pthread_t tid1,tid2;
        if(argc != 2){
            fprintf(stderr,"usage : %s <niters> \n" , argv[0]);
            exit(0);
        }
        niters = atoi(argv[1]);
        Pthread_create(&tid1,NULL,thread,&niters);
        Pthread_create(&tid2,NULL,thread,&niters);
        Pthread_join(tid1,NULL);
        Pthread_join(tid2,NULL);
        if(cnt != (2 *niters)){
            printf("BOOM! cnt=%ld\n",cnt);
            
        }else{
            printf("OK cnt = %ld\n",cnt);
        }
        exit(0);
    }
    void *thread(void *vargp){
        int i;
        int niters = *((long *)vargp);
        for(i = 0 ; i < niters;i++){
            cnt++;
        }
        return NULL;
    }

> ./badcnt 2000
BOOM! cnt=3609

出现这种情况的原因是：一般没有办法预测操作系统是否为你的线程选择一个正确的顺序
cnt++的操作顺序是：
1. 把cnt加载到累加寄存器
2. 更新累加寄存器(cnt)
3. 把累加寄存器的值传回给cnt

但是多线程可能会把不同线程的1 2 3 顺序打乱。

#### 12.5.2 信号量
Edsger Dijkstra，提出了信号量方法解决互斥问题。信号量s是一个非负的全局变量,只能由两种特殊操作处理：P & V
- P(s): 
  如果s是非零的，那么P把s 减 1 ，并且立即返回。
  如果s等于0 ， 那么挂起这个线程，直到s变为非0，而一个V操作会重启这个线程。重启之后，P把s-1,把控制返回给调用者
- V(s):
  V操作把s+1.如果有任何线程阻塞在P，会重启一个阻塞线程。但是我们不能预测V要重启哪个线程

Posix标准定义了许多操作信号量函数

    int sem_init(sem_t *sem , 0 , unsigned int value); // 把信号量sem初始化为value
    int sem_wait(sem_t *s ) P(s)
    int sem_post(sem_t *s ) V(s)

包装函数

    void P(sem_t *s);
    void V(sem_t *s);
#### 12.5.3 使用信号量来实现互斥
把每个共享变量与一个信号量联系起来，然后用P和V操作将相应的临界区包围起来。
这种方式称为二元信号量，因为s值为0 or 1;
以提供互斥为目的的二元信号量常常称为互斥锁，P称为加锁，V解锁

    sem_t mutex;
    Sem_init(&mutex,0,1);
    for(i = 0 ; i < niters;i++){
            P(&mutex);
            cnt++;
            V(&mutex);
        }

#### 12.5.4 利用信号量来调度共享资源
信号量的另一个作用就是调度对共享资源的访问，用信号量来通知另外一个线程。
##### 1. 生产者-消费者问题
生产者和消费者共享一个有n个槽的有限缓冲区，生产者提供items,消费者取出items.
首先我们必须保证对缓冲区的访问是互斥的。
同时我们还需要调度对缓冲区的访问，例如buffer is empty , 生产者需要生产出一个item，消费者才能取出
而buffer is full , 消费者取出一个生产者才能继续生产。

    typedef struct {
        int *buf;
        int n ;
        int front; // buf[(front+1)%n] is the first item
        int rear;   // buf[rear%n] is the last item
        sem_t mutex;    //互锁信号量
        sem_t slots;    //空槽数目
        sem_t items;
    }sbuf_t

    void sbuf_init(sbuf_t *sp, int n)
    {
        sp->buf = Calloc(n, sizeof(int));
        sp->n = n;
        sp->front = sp->rear = 0;
        Sem_init(&sp->mutex, 0, 1);
        Sem_init(&sp->slots, 0, n);
        Sem_init(&sp->items, 0, 0);
    }
    void sbuf_deinit(sbuf_t *sp){
        Free(sp->buf);
    }
    void sbuf_insert(sbuf_t *sp, int item){
        P(&sp->slots);
        P(&sp->mutex);
        sp->buf[(++sp->rear)%(sp->n)] = item;
        V(&sp->mutex);
        V(&sp->items);
    }
    int sbuf_remove(sbuf_t *sp){
        int item;
        P(&sp->items);
        P(&sp->mutex);
        item = sp->buf[(++sp->front)%(sp->n)];
        V(&sp->mutex);
        V(&sp->slots);
        return item;
    }

##### 2. 读者-写者问题
一组并发的线程要访问一个共享对象，有些线程只读对象，称为读者，有些线程只修改对象，称为写者。
###### 第一类读者写者问题：
读者优先，要求不让读者等待，除非写者已经获得了对象的权限
int readcnt; /initially = 0;
sem_t mutex , w ; //initially =1;
void reader(void){
    while(1){
        P(&mutex);
        readcnt++;
        if(readcnt ==  1){
            P(&w);
        }
        V(&mutex);

        <!-- reading happens -->

        P(&mutex);
        readcnt--;
        if(readcnt == 0){
            V(&w);
        }
        V(&mutex);
    }
}
void writer(void){
    while(1){
        P(&w);
        
        <!-- writing happens -->
        V(&w);
    }
}

#### 12.5.5 基于预线程化的并发服务器
    #include "csapp.h"
    #include "sbuf.h"
    #define NTHREADS 4
    #define SBUFSIZE 16

    void echo_cnt(int connfd);
    void *thread(void *vargp);

    sbuf_t sbuf;

    int main(int argc, char *argv[])
    {
        int i, listenfd, connfd;
        socklen_t clientlen;
        struct sockaddr_storage clientaddr;
        pthread_t tid;
        if (argc != 2)
        {
            fprintf(stderr, "usage: %s <port> \n", argv[0]);
            exit(0);
        }
        listenfd = Open_listenfd(argv[1]);
        sbuf_init(&sbuf, SBUFSIZE);
        for (int i = 0; i < NTHREADS; i++)
        {
            Pthread_create(&tid, NULL, thread, NULL);
        }
        while (1)
        {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientlen, &clientlen);
            sbuf_insert(&sbuf, connfd);
        }
    }
    void *thread(void *vargp)
    {
        Pthread_detach(Pthread_self());
        while (1)
        {
            int connfd = sbuf_remove(&sbuf);
            echo_cnt(connfd);
            Close(connfd);
        }
    }
    static int byte_cnt;
    static sem_t mutex;
    static void init_echo_cnt(void)
    {
        Sem_init(&mutex, 0, 1);
        byte_cnt = 0;
    }
    void echo_cnt(int connfd)
    {
        char buf[MAXLINE];
        int n;
        rio_t rio;
        static pthread_once_t once = PTHREAD_ONCE_INIT;
        Pthread_once(&once, init_echo_cnt);
        Rio_readinitb(&rio, connfd);
        while ((n = Rio_readlineb(&rio, buf, MAXLINE)))
        {
            P(&mutex);
            byte_cnt += n;
            printf("Server received %d bytes (%d bytes total )\n" , n , byte_cnt);
            V(&mutex);
            Rio_writen(connfd,buf,n);
        }
    }

### 12.6 使用线程提高并行性
大多数现代机器都有多个核，并发程序在这样的机器上运行地更快，因为内核在多个核上并行的调度这些线程。
并发程序包括并行程序；

    #include "csapp.h"
    #define MAXTHREADS 32
    long gsum = 0;
    long nelems_per_thread;
    sem_t mutex;
    void *sum_mutex(void *var);
    #include <time.h>
    int main(int argc , char *argv[]){
        pthread_t tid[MAXTHREADS];
        long nthreads,log_nelems,i,nelems,myid[MAXTHREADS];
        if(argc != 3){
            fprintf(stderr , "usage : %s <nthreads> <long_nelems>\n",argv[0]);
            exit(0);
        }
        
        clock_t start_t,finish_t;
        double total_t = 0;
        start_t = clock();


        nthreads = atoi(argv[1]);
        log_nelems = atoi(argv[2]);
        nelems = (1L << log_nelems);
        nelems_per_thread = nelems / nthreads;
        sem_init(&mutex,0,1);
        for(i = 0 ; i < nthreads ; i++){
            myid[i] = i;
            Pthread_create(&tid[i],NULL,sum_mutex,&myid[i]);
        }
        for(i = 0 ; i < nthreads ; i++){
            Pthread_join(tid[i],NULL);
        }
        finish_t = clock();
        total_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC;//将时间转换为秒
        printf("CPU 占用的总时间：%f\n", total_t);
        if(gsum != (nelems *(nelems - 1))/2){
            printf("Error : result = %ld\n",gsum);
        }
        exit(0);
    }
    void *sum_mutex(void *var){
        long i,start,end;
        long myid = *((long *)var);
        start = myid * nelems_per_thread;
        end = start + nelems_per_thread;
        for(i = start ; i < end ;i++){
            P(&mutex);
            gsum+=i;
            V(&mutex);
        }
        return NULL;
    }

    jasonj@laptopofjason:~/Desktop/CSAPP/12Concurrent$ ./psum 1 31
    CPU 占用的总时间：21.142562
    jasonj@laptopofjason:~/Desktop/CSAPP/12Concurrent$ ./psum 2 31
    CPU 占用的总时间：270.740275
**同步开销巨大，要尽可能避免，如果无法避免，要用尽可能多的有用计算弥补开销**
