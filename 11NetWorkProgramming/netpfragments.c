#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
int main(){
    char *src = "128.2.194.242";
    struct in_addr dst;
    int r = inet_pton(AF_INET,src,(void *)&dst);
    uint32_t res = ntohl(dst.s_addr);
    char revert[30];
    inet_ntop(AF_INET,(void*)&dst,revert,30);
    printf("0x%x\n",res);
    printf("%s\n",revert);
}

