#include "kernel/types.h"
#include "user/user.h"

void primes(int *);

int main(int argc, char**argv)
{   
    //正确的做法还得是递归比较合理
    int i;
    int p2s_org[2];
    pipe(p2s_org);

    int pid;

    pid=fork();

    if(pid==0){//子进程
        close(p2s_org[1]);//不会写回
        primes(&p2s_org[0]);
    }
    else{//父进程
        close(p2s_org[0]);
        for(i=2;i<32;i++){
            write(p2s_org[1],&i,1);
        }
        i=0;//0是关闭信号，当发送0的时候就告诉子进程该收手了
        write(p2s_org[1],&i,1);
        wait(0);
        exit(0);
    }
    return 0;
}

void primes(int *inp){
    int c;
    read(inp[0],&c,1);
    if(c==0){
        close(inp[0]);
        exit(0);
    }
    printf("prime %d \n",c);
    int fnum=c;

    int p_next[2];
    pipe(p_next);
    
    int pid=fork();
    if(pid==0){
        close(p_next[1]);
        primes(p_next);
    }
    else{
        close(p_next[0]);
        while (1)
        {
            read(inp[0],&c,1);
            if(c%fnum!=0){
                write(p_next[1],&c,1);
            }
            if(c==0){
                write(p_next[1],&c,1);
                close(p_next[1]);
                wait(0);//等待子进程结束
                exit(0);
            }
        }
    }
}
