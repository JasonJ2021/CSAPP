# Shell lab
### Hints
- 阅读教科书
- 使用trace文件来指导shell的编写
- waitpid , kill , fork,execve ,setpgid,sigprocmask会很有用，WUNTRACED and WNOHANG 在waitpid中会有用
- 实现信号处理函数时，把SIGINT and SIGTSTP 传送给所有前台进程组，kill -pid
- 在waitfg中，使用busy-loop around the sleep()
  在sigchld_handler使用waitpid
- 防止父子进程竞争
  
### Tasks
- eval: Main routine that parses and interprets the command line. [70 lines]
- builtin cmd: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs. [25
lines]
- do bgfg: Implements the bg and fg built-in commands. [50 lines]
- waitfg: Waits for a foreground job to complete. [20 lines]
- sigchld handler: Catches SIGCHILD signals. 80 lines]
- sigint handler: Catches SIGINT (ctrl-c) signals. [15 lines]
- sigtstp handler: Catches SIGTSTP (ctrl-z) signals. [15 lines]
  
## eval
    void eval(char *cmdline)
    {
        char *argv[MAXLINE];
        char buf[MAXLINE];
        int bg;
        pid_t pid;
        strcpy(buf, cmdline);
        bg = parseline(buf, argv); //bg = 1 后台运行
        sigset_t mask_all, prev_one, mask_one;
        if (argv[0] == NULL)
            return;
        if (!builtin_cmd(argv))
        {
            sigfillset(&mask_all);
            sigemptyset(&mask_one);
            sigaddset(&mask_one, SIGCHLD);
            sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
            if ((pid = fork()) == 0)
            {
                sigprocmask(SIG_SETMASK, &prev_one, NULL);
                setpgid(0, 0);
                if (execve(argv[0], argv, environ) < 0)
                {
                    printf("%s: Command not found.\n", argv[0]);
                    exit(0);
                }
            }
            sigprocmask(SIG_SETMASK, &mask_all, NULL);
            if (!bg)
            {
                addjob(jobs, pid, FG, cmdline);
            }
            else
            {
                addjob(jobs, pid, BG, cmdline);
            }
            sigprocmask(SIG_SETMASK, &prev_one, NULL);
            if (!bg)
            {
                waitfg(pid);
            }
            else
            {
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
            }
        }
        return;
    }
####Points
- 在fork之前就屏蔽了SIGCHLD防止父子进程相互竞争的情况
- execve(const char *pathname, char *const argv[], char *const envp[]))，函数执行之后不返回，因此如果没有找到指令应该exit(0)
- 加塞操作：
  > sigset_t mask_all, prev_all;
    sigfillset(&mask_all);
    sigprocmask(SIG_SETMASK, &mask_all, NULL);
    要执行的操作
    sigprocmask(SIG_SETMASK, &prev_one, NULL);
- 如果在后台执行，就输出jobid pid 和指令
  如果在前台执行，要等待指令执行完成
  > if (!bg)
            {
                waitfg(pid);
            }


## buildin_cmd
    int builtin_cmd(char **argv)
    {
        if (!strcmp(argv[0], "quit"))
        {
            exit(0);
        }
        if (!strcmp(argv[0], "jobs"))
        {
            listjobs(jobs);
            return 1;
        }
        if (!strcmp(argv[0], "bg"))
        {
            do_bgfg(argv);
            return 1;
        }
        if (!strcmp(argv[0], "fg"))
        {
            do_bgfg(argv);
            return 1;
        }
        return 0; /* not a builtin command */
    }
- 在执行quit的时候，是在tsh的主进程中执行，所以会退出shell进程

## do_bgfg
    void do_bgfg(char **argv)
    {
        struct job_t *jobid;
        if(argv[1] == NULL){
            printf("%s command requires PID or %%jobid argument\n",argv[0]);
            return;
        }
        for(int i = 0 ; i < strlen(argv[1]) ; i++){
            if(isalpha(argv[1][i])){
                printf("%s: argument must be a PID or %%jobid\n",argv[0]);
                return;
            }
        }
        if (!strcmp(argv[0], "bg"))
        {
            int bgid;
            //判断是JID还是PID
            //如果是JID(%xxx)则对整个进程组进行操作，
            //PID对一个进程进行操作
            if (argv[1][0] == '%')
            {
                sscanf(argv[1], "%%%d", &bgid);
                if ((jobid = getjobjid(jobs, bgid)) == NULL)
                {
                    printf("%d: No such job\n",bgid);
                    return;
                }
                jobs[jobid->jid-1].state = BG;
                kill(-(jobid->pid), SIGCONT);
                printf("[%d] %s", jobid->pid, jobid->cmdline);
                return;
            }
            else
            {
                sscanf(argv[1], "%d", &bgid);
                if ((jobid = getjobpid(jobs, bgid)) == NULL)
                {
                    printf("(%d): No such process\n",bgid);
                    return;
                }
                jobs[jobid->jid-1].state = BG;
                kill(-jobid->pid, SIGCONT);
                printf("[%d] %s", jobid->pid, jobid->cmdline);
                return;
            }
        }
        else
        {
            int fgid;
            //判断是JID还是PID
            //如果是JID(%xxx)则对整个进程组进行操作，
            //PID对一个进程进行操作
            if (argv[1][0] == '%')
            {
                sscanf(argv[1], "%%%d", &fgid);
                if ((jobid = getjobjid(jobs, fgid)) == NULL)
                {
                    printf("%d: No such job\n",fgid);
                    return;
                }
                kill(-(jobid->pid), SIGCONT);
                jobs[jobid->jid-1].state = FG;
                waitfg(0);
            
                return;
            }
            else
            {
                sscanf(argv[1], "%d", &fgid);
                if ((jobid = getjobpid(jobs, fgid)) == NULL)
                {
                    printf("(%d): No such process\n",fgid);
                    return;
                }
                kill(-jobid->pid, SIGCONT);
                jobs[jobid->jid-1].state = FG;
                waitfg(0);
                return;
            }
        }
        return;
    }

#### Points
- bg %1 和 bg 1 是不一样的
  %1代表job1 有可能是一个进程组，而 1 则代表了一个进程
- 事实上所有在子进程对全局数据结构的访问都要加塞..
- kill函数要先于waitfg函数 发出信号
- 注意jobs[jid - 1]才是正确的job索引

## waitfg

    void waitfg(pid_t pid)
    {
        sigset_t mask;
        sigemptyset(&mask);
        while (fgpid(jobs) > 0)
        {
            sigsuspend(&mask);
        }
        return;
    }

#### Points
- 接受一次信号就会进行一次循环，这里挂起了shell的主进程，等待fg process发送信号，进行判断

## sigchld_handler
    void sigchld_handler(int sig)
    {
        int olderrno = errno;
        sigset_t mask_all, prev_all;
        pid_t pid;
        sigfillset(&mask_all);
        int status;
        //注意每次process终止，中断，恢复都会发送sigchld给父进程
        while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
        {
            if (WIFEXITED(status))
            {
                sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
                deletejob(jobs, pid);
                sigprocmask(SIG_SETMASK, &prev_all, NULL);
            }
            else if (WIFSIGNALED(status))
            {
                struct job_t *jobid;
                jobid = getjobpid(jobs, pid);
                sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
                printf("Job [%d] (%d) terminated by signal %d\n", jobid->jid, jobid->pid, SIGINT);
                deletejob(jobs, jobid->pid);
                sigprocmask(SIG_SETMASK, &prev_all, NULL);
            }
            else if (WIFSTOPPED(status))
            {
                sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
                int jid = pid2jid(pid);
                jobs[jid-1].state = ST;
                printf("Job [%d] (%d) stopped by signal %d\n", jid, pid, SIGTSTP);
                sigprocmask(SIG_SETMASK, &prev_all, NULL);
            }
        }
        errno = olderrno;
        return;
    }

#### Points
**子进程如果终止 或 中断 或 恢复 都会发送给父进程一个SIGCHLD信号**
**sigint_handler and SIGTSTP_handler 是用来处理对shell发送的信号**
**sigchld_handler是用来处理shell子进程对shell发送的信号**
- WNOHANG 如果等待集合中的任何子进程都还没有返回，那么立即返回（返回值默认为0)
  WUNTRACED 挂起调用进程的执行，直到等待集合中的一个进程变成已终止，返回那个进程的pid
  WNOHANG | WUNTRACED 立即返回，返回值为0或者终止子进程的pid
- 要对不同子进程SIGCHLD的行为进行分别处理
  WIFEXITED(status) : 检验子进程是否通过调用exit或者return终止
  这时候什么都不输出，只是在joblist中delete这个进程
  WIFSIGNALED(STATUS) : 检验子进程是否是被一个未blocked的信号终止 ， 这时候在delete并且输出一条信息
  WIFSTOPPED(STATUS) : 检验子进程是否是被暂停，这时候只需要修改job的状态

## sigint_handler
    void sigint_handler(int sig)
    {
        int olderrno = errno;
        pid_t forepid = fgpid(jobs);
        if (forepid != 0)
        {
            kill(-forepid, SIGINT);
        }
        errno = olderrno;
        return;
    }