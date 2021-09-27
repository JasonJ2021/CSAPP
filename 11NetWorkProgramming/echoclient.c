#include "csapp.h"

int main(int argc , char *argv[]){
    if(argc != 3 ){
        fprintf(stderr,"usage : %s <hostname> <port> \n",argv[0]);
        exit(0);
    }
    int clientfd ; 
    char *host,*port,buf[MAXLINE];
    rio_t rio;
    host = argv[1];
    port = argv[2];
    clientfd = Open_clientfd(host,port);
    Rio_readinitb(&rio ,clientfd);
    while(Fgets(buf,MAXLINE , stdin) != NULL){
        Rio_writen(clientfd , buf,strlen(buf));
        Rio_readlineb(&rio , buf , MAXLINE);
        Fputs(buf , stdout);
    }
    Close(clientfd);
    exit(0);
}