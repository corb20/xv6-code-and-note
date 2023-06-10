#include<kernel/types.h>
#include<user/user.h>

int main(int argc,char** argv)
{
    if(argc!=2)
    {
        printf("Error Example:sleep 2\n");
        exit(-1);
    }

    //sleep系统调用函数在在user.h中被定义，但深层是通过汇编语言绑定到sys_sleep函数上的
    int num_of_tick = atoi(argv[1]);
    if(sleep(num_of_tick)<0)
    {
        printf("Can not sleep\n");
        exit(-1);
    }
    exit(0);
}