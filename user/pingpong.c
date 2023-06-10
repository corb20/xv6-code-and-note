#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv)
{
    int pTos[2];
    int sTop[2];
    int p_pid;
    int pid;
    int n;

    pipe(pTos);
    pipe(sTop);

    p_pid = getpid();
    if( (pid= fork())<0 )
    {
        printf("fork failed\n");
        exit(-1);
    }
    if(pid== 0)//此处为子进程
    {
        close(pTos[1]);
        close(sTop[0]);
        char c = 'a';
        if((n=write(sTop[1], &c, 1))<0)
        {
            printf("write failed\n");
            exit(-1);
        }
        c = '\0';
        if ((n = read(pTos[0], &c, 1)) < 0)
        {
            printf("read error");
            exit(-1);
        }
        int spid=getpid();
        if (n > 0 && c!='\0')
            printf("<%d>:received ping\n", spid);
        exit(0);
    }
    //此处为父进程
    close(pTos[0]);
    close(sTop[1]);
    char c = 'a';
    if((n=write(pTos[1], &c, 1))<0)
    {
        printf("write failed\n");
        exit(-1);
    }
    //父进程发送完消息之后就需要陷入等待状态
    wait(0);
    c = '\0';//父进程的打印要在子进程的打印之后结束，否则就会导致打印出来的东西乱七八糟（因为时间片轮转算法）
    if ((n = read(sTop[0], &c, 1)) < 0)
    {
        printf("read error");
        exit(-1);
    }
    if (n > 0 && c!='\0')
        printf("<%d>:received pong\n", p_pid);
    exit(0);
}