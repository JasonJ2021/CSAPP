#include "csapp.h"
void echo(int connfd);

int main(int argc , char *argv[]){
    int listenfd , connfd ; 
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE],client_portname[MAXLINE];
    if( argc != 2){
        fprintf(stderr , "usage : %s <port>" , argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd,(SA *) &clientaddr ,&clientlen);
        Getnameinfo((SA*)&clientaddr,clientlen,client_hostname,MAXLINE,client_portname,MAXLINE , 0);
        printf("Connected to (%s %s)\n",client_hostname , client_portname);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}