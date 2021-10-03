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
