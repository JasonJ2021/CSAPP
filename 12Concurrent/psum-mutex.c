#include "csapp.h"
#define MAXTHREADS 32
long gsum = 0;
long nelems_per_thread;
sem_t mutex;
void *sum_mutex(void *var);
static long psum[MAXTHREADS];
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
    for(i = 0 ; i < nthreads ; i++){
        gsum+=psum[i];
    }
    finish_t = clock();
    total_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC;//将时间转换为秒
    printf("CPU 占用的总时间：%lf\n", total_t);
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
    long sum= 0;
    for(i = start ; i < end ;i++){
        sum+=i;
    }
    psum[myid] = sum;
    return NULL;
}