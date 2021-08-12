long mult2(long,long);

void multstore(long x , long y , long *dst){
    long t = mult2(x,y);
    *dst = t;
}