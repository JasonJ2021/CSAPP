编译时记得加 **-lpthread**

- [11.4 套接字接口～](#114-套接字接口)
  - [11.4.1 套接字地址结构](#1141-套接字地址结构)
  - [11.4.2 socket函数](#1142-socket函数)
  - [11.4.3 connect函数](#1143-connect函数)
  - [11.4.4 bind函数](#1144-bind函数)
  - [11.4.5 listen函数](#1145-listen函数)
  - [11.4.6 accept函数](#1146-accept函数)
  - [11.4.7 主机和服务的转换](#1147-主机和服务的转换)
    - [1.getaddrinfo函数](#1getaddrinfo函数)
    - [2.getnameinfo函数](#2getnameinfo函数)
  - [11.4.8 套接字接口的辅助函数](#1148-套接字接口的辅助函数)
    - [1.open_clientfd函数](#1open_clientfd函数)
    - [2.open_listenfd函数](#2open_listenfd函数)
  
---

### 11.3.1 IP地址
一个IP地址是32位无符号整数

    struct in_addr{
        uint32_t s_addr;
    };
TCP/IP定义了network byte order(大端字节顺序)

Unix提供了以下函数在主机与网络之间进行字节顺序的转换

    #include <arpa/inet.h>
    uint32_t htonl(uint32_t hostlong); //host converts to hbo long
    uint16_t htons(uint16_t hostshort);

    uint32_t ntohl(uint32_t netlong)
    uint16_t ntohs(uint16_t netshort);

IP地址通常是用点分十进制表示法表示的，128.2.194.242 代表0x8002c2f2
在Linux系统上，可以使用HOSTNAME命令确定IP地址

    jasonj@laptopofjason:~$ hostname -i
    127.0.1.1

应用程序使用inet_pton和inet_ntop来实现IP和点分十进制串之间的转换(**注意函数处理的结果uint32_t可能是小端法,要适当转换**)

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
    >>>结果如下
    jasonj@laptopofjason:~/Desktop/CSAPP/11NetWorkProgramming$ ./net
    0x8002c2f2
    128.2.194.242

### 11.3.2 因特网域名

ip地址很难记住，所以internet定义了一组domain name ,
域名集合形成一个层次结构，第一层是一个未命名的根节点，下一层是一级域名(first-level domain name),包含com,edu,gov,org,net等等
下一层是二级域名例如 cmu.edu, mit.edu , amazon.com。一个组织如果获得了二级域名，就可以在这个子域创建任何新的域名，如 cs.cmu.edu

因特网定义了域名集合和IP地址集合之间的映射，现在它通过DNS（DOMAIN NAME SYSTEM）维护，每条定义了一组域名和一组IP地址之间的映射。
我们可以使用nslookup来查看一些域名信息
每台主机都有本地域名localhost,总是映射为127.0.0.1
在某种情况下，多个域名可以映射为同一个IP地址,例如

    jasonj@laptopofjason:~/Desktop/CSAPP/11NetWorkProgramming$ nslookup cs.mit.edu
    Server:		127.0.0.53
    Address:	127.0.0.53#53

    Non-authoritative answer:
    cs.mit.edu	canonical name = eecs.mit.edu.
    Name:	eecs.mit.edu
    Address: 18.25.0.23

### 11.3.3 因特网连接
客户端和服务端通过在*连接*是嗯发送和接受字节流来通信，他们是点对点，双全工，可靠的。
一个套接字是连接的一个端点，它有套接字地址，由因特网地址和一个16位的整数端口组成，用“地址：端口”表示。
客户端发起请求的时候，套接字端口是内核自动分配的。但是服务器有规定.Web服务器使用端口80,emain server使用25.文件etc/services包含这台机器提供的知名名字和端口之间的映射。
连接是由一个套接字对决定的：
(cliaddr:cliport,servaddr,servport)
## 11.4 套接字接口～
### 11.4.1 套接字地址结构
从Linux内核的角度来看，一个套接字就是通信的一个端点，从Linux程序的角度看，套接字就是一个有相应描述符的打开文件。
因特网的套接字地址存放在sockaddr_in的16字节结构中

    struct sockaddr_in {
        uint16_t sin_family; //protocal family always AF_INET
        uint16_t sin_port;  //Port number
        struct in_addr sin_addr;
        unsigned char sin_zero[8]; // Pad to sizeof(struct sockaddr)
    };

    struct sockaddr {
        uint16_t sa_family;
        char sa_data[14];
    };

IP地址和端口号都要求按大端法存放
_in 代表internet
connect , bind,accept都要求一个指向套接字地址结构的指针，但是如何使其接受各种类型的套接字地址结构？如今可以使用void *,当时定义了sockaddr ，把所有特定结构的指针转换成这个通用的结构.
> typedef struct sockaddr SA

### 11.4.2 socket函数
    int socket(int domain , int type , int protocol);
如果想要使套接字成为连接的一个端口

    clientfd = socket(AF_INET,SOCK_STREAM , 0);
SOCK_STREAM代表这个套接字是一个连接的节点，最好的方式是使用getaddrinfo.
socket返回的描述符是部分打开的，不能用于读写，如何打开取决于客户端/服务端

### 11.4.3 connect函数
    int connect(int clientfd , const struct sockaddr_in *addr , socklen_t addrlen);
这时候connect函数正在尝试与套接字地址为addr的服务器建立连接,addrlen为sizeof(sockaddr_in).connect函数会阻塞，一直到连接成功或者发生错误
成功为0，错误返回-1
### 11.4.4 bind函数
    int bind(int sockfd , const struct sockaddr_in *addr , socklen_t addrlen);
bind 函数告诉内核把addr中的服务器套接字地址和套接字描述符sockfd联系起来,addrlen = sizeof(sockaddr_in)
### 11.4.5 listen函数
    int listen(int sockfd , int backlog);
客户端是发起连接请求的主动实体，服务器是等待的被动实体。内核会默认认为socket返回的描述符为主动套接字，而listen告诉内核这个描述符是被服务器使用的。
backlog一般把他设置为一个较大的数例如1024

### 11.4.6 accept函数
    int accept(int listenfd , struct sockaddr *addr , int *addrlen);
accept等待客户端请求到达listenfd,然后在addr中填写客户端的套接字地址，并且返回一个连接套接字
关于为什么要区分连接套接字和监听套接字，主要是为了实现并发服务器。

### 11.4.7 主机和服务的转换
#### 1.getaddrinfo函数
    int getaddrinfo(const char *host,const char *service , const struct addrinfo *hints,struct addrinfo **result);

    void freeaddrinfo(struct addrinfo *result);

    const char *gai_strerror(int errcode);
    ----------------------
    成功返回0 否则返回非零的数;

给定host 和service 事实上只需要给定一个就可以，另外一个设置为空指针
getaddrinfo返回result,指向一个addrinfo的链表，其中每个结构指向一个对应于host和service的套接字接口
最后要把链表空间释放，使用freeaddrinfo
如果出现了错误信息，可以使用gai_strerror打印错误信息.
host可以是域名也可以是数字地址，service也是这样
hints是一个addrinfo结构，提供对getaddrinfo返回的套接字地址链表更好的控制。如果要传递hints参数，只能设置：ai_family、ai_socktype、ai_protocol和ai_flags。其他必须设置为0.
我们可以使用memset函数来把整个结构清零.
- getaddrinfo可以返回ipv4 / ipv6地址，设置ai_family 为AF_INET / AF_INET6
- 把ai_socktype设置为SOCK_STREAM限定列表为连接
- ai_flags是一个位掩码，以下是推荐值
  - AI_ADDRCONFIG   推荐
  - AI_CANONNAME 把第一个addrinfo的ai_canonname指向host的官方名字
  - AI_PASSIVE  告诉函数返回的套接字地址可能是服务端的，在这种情况下,host被设置为NULL
  - AI_NUMERICSERV 强制service为端口号
  
        struct addrinfo{
            int ai_flags;
            int ai_family;
            int ai_socktype;
            int ai_protocol;
            char *ai_canonname;
            size_t ai_addrlen;
            struct sockaddr *ai_addr;
            struct addrinfo *ai_next;
        }
#### 2.getnameinfo函数
    int getnameinfo(const struct sockaddr *sa , socklen_t salen , 
                    char *host , size_t hostlen,
                    char *service , size_t servlen,int flags);
sa指向大小为salen字节的套接字地址结构
host是一个hostlen字节的缓冲区，service同理。
getnameinfo把sa转换成对应的主机和服务字符串复制到相应缓冲区.

flags 是一个掩码
- NI_NUMERICHOST getnameinfo默认返回host中的域名。设置该标志会使其返回一个数字字符串
- NI_NUMERICSERV getnameinfo默认会检查/etc/services，返回服务名。设置该标志会返回端口号
### 11.4.8 套接字接口的辅助函数
#### 1.open_clientfd函数
    int open_clientfd(char *hostname , char *service){
        int clientfd ; 
        struct addrinfo *p , *listp , hint;
        memset(&hint , 0 , sizeof(struct addrinfo));
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV
        Getaddrinfo(host,service , &hint , &listp);
        for(p = listp ; p;p = p->ai_next){
            if((clientfd = socket(p->family , p->socktype , p->protocol))< 0){
                continue ; 
            }
            if((connect(clientfd,p->ai_addr , p->ai_addrlen)) == 0){
                break;
            }
            Close(clientfd);
        }
        Freeaddrinfo(listp);
        if(!p){
            return -1;
        }else{
            return clientfd;
        }
    }
----
#### 2.open_listenfd函数
    int open_listenfd(char *service){
        int listenfd ; 
        struct addrinfo *p , *listp , hint;
        memset(&hint , 0 , sizeof(struct addrinfo));
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV | AI_PASSIVE;
        Getaddrinfo(NULL ,service , &hint , &listp);
        for(p = listp ; p;p = p->ai_next){
            if((listenfd  = socket(p->family , p->socktype , p->protocol))< 0){
                continue ; 
            }
            Setsockopt(listenfd , SOL_SOCKET,SO_REUSEADDR , (const void *)&optval , sizeof(int));
            if((bind(listenfd,p->ai_addr , p->ai_addrlen)) == 0){
                break;
            }
            Close(clientfd);
        }
        Freeaddrinfo(listp);
        if(!p){
            return -1;
        }
        if(listen(listenfd , 1024) < 0 ){
            Close(listenfd );
            return -1;
        }else{
            return listenfd ;
        }
    }