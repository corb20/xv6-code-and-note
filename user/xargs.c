#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char **argv)
{
    //要执行的指令的是argv[1]
    char inputBuf[512];
    char *cmdArgv[32];//执行命令的参数
    char argvBuf[512];
    int argvBufSize;
    char *argvbufp;
    int i = 0;
    int argvi = 0;
    int n;
    int pid;

    for (argvi = 0; argvi < argc-1; argvi++)
    {
        cmdArgv[argvi] = argv[argvi + 1];
    }

    argvBufSize = 0;
    argvbufp = argvBuf;
    while ((n=read(0,inputBuf,512))>0)
    {
        for (i = 0; i < n;i++)
        {
            char curChar = inputBuf[i];
            if (curChar == ' ')
            {
                argvBuf[argvBufSize++] = '\0';
                cmdArgv[argvi] = argvbufp;
                argvi++;
                argvbufp = &argvBuf[argvBufSize];
            }
            else if(curChar=='\n' || curChar=='\r')
            {
                argvBuf[argvBufSize] = '\0';
                cmdArgv[argvi] = argvbufp;
                
                if((pid=fork())<0)
                {
                    printf("Failed to fork\n");
                    exit(-1);
                }
                else if(pid==0)//子进程
                {
                    exec(argv[1], cmdArgv);
                }
                wait(0);
                //父进程继续执行
                argvBufSize = 0;
                argvi = argc - 1;
                argvbufp = argvBuf;
            }
            else
            {
                argvBuf[argvBufSize++] = curChar;
            }
        }
    }
    exit(0);
}