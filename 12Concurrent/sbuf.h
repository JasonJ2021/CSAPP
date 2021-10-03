#include "csapp.h"
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
typedef struct
{
    int *buf;
    int n;
    int front;   // buf[(front+1)%n] is the first item
    int rear;    // buf[rear%n] is the last item
    sem_t mutex; //互锁信号量
    sem_t slots; //空槽数目
    sem_t items;
} sbuf_t;