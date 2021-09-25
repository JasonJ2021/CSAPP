int main()
{
    char c;
    while (read(STDIN_FILENO, &c, 1) != 0)
    {
        write(STDOUT_FILENO, &c, 1);
    }
    exit(0);
}

ssize_t rio_readn(int fd, void *buf, size_t n)
{
    size_t nleft = n;
    char *temp = buf;
    ssize_t nread = 0;
    while (nleft > 0)
    {
        if ((nread = read(fd, temp, nleft)) < 0)
        {
            if (error = EINTR)
            {
                nread = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            break;
        }
        nleft -= nread;
        temp += nread;
    }
    return n - nleft; // 实际传送的数值
}

ssize rio_writen(int fd, void *buf, size_t n)
{
    size_t nleft = n;
    char *temp = buf;
    ssize_t nwrite = 0;
    while (nleft > 0)
    {
        if ((nwrite = write(fd, temp, nleft)) <= 0)
        {
            if (error = EINTR)
            {
                nwrite = 0;
            }
            else
            {
                return -1;
            }
        }
        nleft -= nwrite;
        temp += nwrite;
    }
    return n;
}
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd; //这个缓冲区连接的文件描述符
    int rio_cnt;//缓冲区还没读取的字节个数
    char *rio_bufptr;//缓冲区还没读取字节的开头指针
    char rio_buf[RIO_BUFSIZE]; 缓冲区
}rio_t;

void rio_readinitb(rio_t *rp,int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
int main(){
    int n ;
    rio_t rio;
    char buf[MAXLINE];
    rio_readinitb(&rio,STDIN_FILENO);
    while((n = rio_readlineb(&rio,buf,MAXLINE)) != 0){
        rio_writen(STDOUT_FILENO,buf,n);
    }
}
static ssize_t rio_read(rio_t *rp , char *usrbuf , size_t n){
    int cnt;
    while(rp->rio_cnt <= 0){
        rp->rio_cnt = read(rp->rio_fd,rp->rio_buf , sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0){
            if(errno != EINTR){
                return -1;
            }
        }else if(rp->rio_cnt == 0){
            return 0;
        }else{
            rp->rio_bufptr = rp->rio_buf;
        }
    }
    cnt = n ;
    if(rp->rio_cnt < n){
        cnt = rp->rio_cnt;
    }
    memcpy(usrbuf , rp->rio_bufptr,cnt);
    rp->rio_bufptr +=cnt;
    rp->rio_cnt -=cnt;
    return cnt;
}
static ssize_t rio_readlineb(rio *rp , char *usrbuf , size_t maxlen){
    int n ,rc;
    char c , *bufp = usrbuf;
    for(n =  1 ; n < maxlen ; n++){
        if((rc = rio_read(rp , &c , 1)) == 1){
            *bufp++ =c ;
            if(c == '\n'){
                n++;
                break;
            }els
        }else if(rc == 0){
            if(n == 1){
                return 0;
            }else{
                break;

            }
        }else{
            return -1;
        }
    }
    *bufp = 0;
    return n-1;
}