#include "csapp.h"
#include "sbuf.h"
#define NTHREAD 4
#define SBUFSIZE 16
sbuf_t sbuf;
void * thread(void *var);
void echo_cnt(int connfd);
int main(int argc , char *argv[]){
    int listenfd , connfd;
    pthread_t tid;
    struct sockaddr_storage clientaddr;
    int clientlen;
    if(argc != 2){
        fprintf(stderr , "usage : %s <port> \n",argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf,SBUFSIZE);
    for(int i = 0 ; i < NTHREAD ; i++){
        Pthread_create(&tid,NULL,thread,NULL);
    }
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd , (SA *)&clientaddr , &clientlen);
        sbuf_insert(&sbuf,connfd);
    }
}
void *thread(void *var){
    Pthread_detach(Pthread_self());
    int connfd;
    while(1){
        connfd = sbuf_remove(&sbuf);
        echo_cnt(connfd);
        Close(connfd);
    }
}

static int byte_cnt;
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static sem_t mutex;
void init_thread(void){
    byte_cnt = 0;
    Sem_init(&mutex,0,1);
}
void echo_cnt(int connfd){
    char buf[MAXLINE];
    rio_t rio;
    pthread_once(&once_control , init_thread);
    int n;
    Rio_readinitb(&rio,connfd);

    while((n = Rio_readlineb(&rio,buf,MAXLINE))){
        P(&mutex);
        byte_cnt+=n;
        printf("Server received %d bytes (%d bytes total)\n in %d connfd",n , byte_cnt , connfd);
        V(&mutex);
        Rio_writen(connfd,buf,n);
    }
}
