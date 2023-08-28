# lab2 syscall 实验报告

## 1. system call tracing

```实验要求（英文）：```

In this assignment you will add a system call tracing feature that may help you when debugging later labs. You'll create a new trace system call that will control tracing. It should take one argument, an integer "mask", whose bits specify which system calls to trace. For example, to trace the fork system call, a program calls trace(1 << SYS_fork), where SYS_fork is a syscall number from kernel/syscall.h. You have to modify the xv6 kernel to print out a line when each system call is about to return, if the system call's number is set in the mask. The line should contain the process id, the name of the system call and the return value; you don't need to print the system call arguments. The trace system call should enable tracing for the process that calls it and any children that it subsequently forks, but should not affect other processes.

We provide a trace user-level program that runs another program with tracing enabled (see user/trace.c). When you're done, you should see output like this:

```sh
$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0
$
$ trace 2147483647 grep hello README
4: syscall trace -> 0
4: syscall exec -> 3
4: syscall open -> 3
4: syscall read -> 1023
4: syscall read -> 966
4: syscall read -> 70
4: syscall read -> 0
4: syscall close -> 0
$
$ grep hello README
$
$ trace 2 usertests forkforkfork
usertests starting
test forkforkfork: 407: syscall fork -> 408
408: syscall fork -> 409
409: syscall fork -> 410
410: syscall fork -> 411
409: syscall fork -> 412
410: syscall fork -> 413
409: syscall fork -> 414
411: syscall fork -> 415
...
$   
```

In the first example above, trace invokes grep tracing just the read system call. The 32 is *1<<SYS_read*. In the second example, trace runs grep while tracing all system calls; the 2147483647 has all 31 low bits set. In the third example, the program isn't traced, so no trace output is printed. In the fourth example, the fork system calls of all the descendants of the forkforkfork test in usertests are being traced. Your solution is correct if your program behaves as shown above (though the process IDs may be different).

Some hints:

+ Add $U/_trace to UPROGS in Makefile

+ Run make qemu and you will see that the compiler cannot compile user/trace.c, because the user-space stubs for the system call don't exist yet: add a prototype for the system call to user/user.h, a stub to user/usys.pl, and a syscall number to kernel/syscall.h. The Makefile invokes the perl script user/usys.pl, which produces user/usys.S, the actual system call stubs, which use the RISC-V ecall instruction to transition to the kernel. Once you fix the compilation issues, run trace 32 grep hello README; it will fail because you haven't implemented the system call in the kernel yet.

+ Add a sys_trace() function in kernel/sysproc.c that implements the new system call by remembering its argument in a new variable in the proc structure (see kernel/proc.h). The functions to retrieve system call arguments from user space are in kernel/syscall.c, and you can see examples of their use in kernel/sysproc.c.

+ Modify fork() (see kernel/proc.c) to copy the trace mask from the parent to the child process.

+ Modify the syscall() function in kernel/syscall.c to print the trace output. You will need to add an array of syscall names to index into.


```实验要求（中文）：```

在这个作业中，您将添加一个系统调用跟踪功能，该功能可以帮助您在调试后续实验时进行调试。您将创建一个新的跟踪系统调用，该系统调用将控制跟踪。它应该有一个参数，一个整数“mask”，其位指定要跟踪的系统调用。例如，要跟踪fork系统调用，程序调用trace（1 << SYS_fork），其中 *SYS_fork* 是来自 *kernel/syscall.h* 的 *syscall* 号。您必须修改xv6内核以在每个系统调用即将返回时打印一行（如果系统调用号在掩码中设置）。该行应包含进程id，系统调用的名称和返回值；您不需要打印系统调用参数。trace系统调用应该为调用它的进程以及它随后fork的任何子进程启用跟踪，但不应影响其他进程。

这个其实就是为进程设置一个跟踪syscall的掩码，当调用调用相应的掩码时，会自动打印出相应的信息。

在第一个示例中，*trace* 调用 *grep* 跟踪只读系统调用。 32是 *1 << SYS_read*。 在第二个示例中，trace运行grep，同时跟踪所有系统调用; 2147483647具有所有31个低位设置。 在第三个示例中，程序没有被跟踪，因此不会打印任何跟踪输出。 在第四个示例中，正在跟踪forkforkfork测试的所有后代的fork系统调用。 如果您的程序的行为如上所示（尽管进程ID可能不同），则您的解决方案是正确的。

一些提示：
+ 将UPROGS中的$U/_trace添加到Makefile中

+ 运行make qemu，你会发现编译器无法编译user/trace.c，因为系统调用的用户空间存根还不存在:向user/user.h添加一个系统调用原型，向user/usys.pl添加一个存根，向kernel/syscall.h添加一个系统调用编号。Makefile调用perl脚本user/usys.pl，它生成user/usys.S，实际上的系统调用存根，它使用RISC-V ecall指令转换到内核。一旦你修复了编译问题，运行trace 32 grep hello README;它会失败，因为你还没有在内核中实现系统调用。

+ 在kernel/sysproc.c中添加一个sys_trace()函数来实现新的系统调用，方法是在proc结构中记住它的参数(参见kernel/proc.h)。从用户空间获取系统调用参数的功能位于kernel/syscall.c中，你可以在kernel/sysproc.c中看到它们的使用示例。

+ 修改fork()(参见kernel/proc.c)，将跟踪掩码从父进程复制到子进程。

+ 修改kernel/syscall.c中的syscall()函数以打印跟踪输出。你需要添加一个系统调用名称数组以索引到。

```实验代码如下：```

针对该实验进行分析，首先是要实现trace的系统调用
那么需要在`user.h`中加入系统调用的声明
```c
...
int trace(int mask);
```
然后在`usys.pl`中加入系统调用的声明
```c
...
entry("trace");
```
理解usys.pl他是编译出一个汇编文件，将系统调用的参数放入寄存器中，然后调用ecall指令，进入内核态，然后在syscall.c中加入系统调用的编号
然后要在`syscall.h`中加入系统调用的编号
```c
...
#define SYS_trace 22
```

然后在`syscall.c`中用extern实现系统调用的声明，并将该函数指针加入到系统调用的列表中去
```c
...
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
extern uint64 sys_trace(void);
...

static uint64 (*syscalls[])(void) = {
...
[SYS_trace]   sys_trace,
...
};

//还要定义相应的系统调用的名称，方便后面输出调用
char *syscallDict[] = {"sys_fork",
                       "sys_exit",
                       "sys_wait",
                       "sys_pipe",
                       "sys_read",
                       "sys_kill",
                       "sys_exec",
                       "sys_fstat",
                       "sys_chdir",
                       "sys_dup",
                       "sys_getpid",
                       "sys_sbrk",
                       "sys_sleep",
                       "sys_uptime",
                       "sys_open",
                       "sys_write",
                       "sys_mknod",
                       "sys_unlink",
                       "sys_link",
                       "sys_mkdir",
                       "sys_close",
                       "sys_trace",
                       "sys_info"};

```
在实现sys_trace函数之前，先要在`proc.h`中加入一个变量，用来存储掩码
```c
struct proc {
  struct spinlock lock;
  ...
  int mask;                    //用于trace的掩码
};
```

并且在`proc.c`的`fork`这一系统调用中让子进程可以继承父进程的掩码
```c
int
fork(void)
{
  ...
  safestrcpy(np->name, p->name, sizeof(p->name));

  np->mask = p->mask;
  
  pid = np->pid;
  ...
}
```

然后在`sysproc.c`中实现系统调用的具体功能
```c
uint64
sys_trace(void)
{
  int mask;
  if(argint(0, &mask)<0)
    return -1;
  struct proc *p = myproc();
  p->mask = mask;
  return 0;
}
```

要让trace的系统调用生效，还需要在`syscall.c`中实现trace的功能,即当执行系统调用时，对比系统调用码与掩码mask，如果比对成功（做&运算），则打印出相应的信息
```c
void
syscall(void)
{
  int num;
  int mask;
  struct proc *p = myproc();
  int pid = p->pid;

  num = p->trapframe->a7;
  mask = p->mask;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
    if(mask & (1<<num))
    {
      printf("%d : syscall %s -> %d \n", pid,syscallDict[num-1], p->trapframe->a0);
    }
  } 
  ...
  
}
```

## 2.Sysinfo

```实验要求（英文）：```

In this assignment you will add a system call, sysinfo, that collects information about the running system. The system call takes one argument: a pointer to a struct sysinfo (see kernel/sysinfo.h). The kernel should fill out the fields of this struct: the freemem field should be set to the number of bytes of free memory, and the nproc field should be set to the number of processes whose state is not UNUSED. We provide a test program sysinfotest; you pass this assignment if it prints "sysinfotest: OK".

Some hints:

+ Add $U/_sysinfotest to UPROGS in Makefile

+ Run make qemu; user/sysinfotest.c will fail to compile. Add the system call sysinfo, following the same steps as in the previous assignment. To declare the prototype for sysinfo() in user/user.h you need predeclare the existence of struct sysinfo:

```c
struct sysinfo;
int sysinfo(struct sysinfo *);
```

Once you fix the compilation issues, run sysinfotest; it will fail because you haven't implemented the system call in the kernel yet.
+ sysinfo needs to copy a struct sysinfo back to user space; see sys_fstat() (kernel/sysfile.c) and filestat() (kernel/file.c) for examples of how to do that using copyout().

+ To collect the amount of free memory, add a function to kernel/kalloc.c

+ To collect the number of processes, add a function to kernel/proc.c

```实验要求（中文）：```

 在本次作业中，您将添加一个系统调用 *sysinfo*，该系统调用收集有关正在运行的系统的信息。这个系统调用需要一个参数：一个指向sysinfo结构体的指针（参见kernel/sysinfo.h）。内核应该填充这个结构体的字段：freemem字段应该被设置为空闲内存的字节数，nproc字段应该被设置为状态不是UNUSED的进程数。我们提供了一个测试程序sysinfotest；如果它打印出“sysinfotest: OK”，则通过此作业。

一些提示：

+ 在Makefile中，向UPROGS添加$U/_sysinfotest

+ 运行make qemu; user/sysinfotest.c将无法编译。添加系统调用sysinfo，按照上一个作业中的相同步骤进行操作。在user/user.h中声明sysinfo()的原型，您需要预先声明struct sysinfo的存在：

```c
struct sysinfo;
int sysinfo(struct sysinfo *);
```

修复编译问题后，运行sysinfotest; 将失败，因为您尚未在内核中实现系统调用。
+ sysinfo需要将一个struct sysinfo复制回用户空间; 查看sys_fstat() (kernel/sysfile.c)和filestat() (kernel/file.c)的示例，了解如何使用copyout()来执行该操作。

+ 要收集可用内存的数量，请将一个函数添加到kernel/kalloc.c

+ 要收集进程的数量，请将一个函数添加到kernel/proc.c 

```实验思路：```

添加基本的syscall的框架与trace类似，以下为基本代码
usys.pl
```c
...
entry("trace");
```
syscall.h
```c
#define SYS_sysinfo 22
```
syscall.c
```c
extern uint64 sys_info(void);

static uint64 (*syscalls[])(void) = {
...
[SYS_sysinfo]   sys_info,
...
};
```
观察sysinfo的要求
```c
struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};
```

在sysproc.c中实现系统调用的具体功能
值得注意的是，该系统调用是通过传入sysinfo的指针，然后让内核填充sysinfo的字段，因此需要用到copyout函数，copyout函数是将内核空间中的数据拷贝到用户空间的函数，在hint中提示了可以观察sys_fstat()函数进行学习

sysproc.c
```c
uint64
sys_info(void)
{
  struct proc *p;
  struct sysinfo info;
  uint64 addr;
  if (argaddr(0, &addr) < 0)
    return -1;
  p = myproc();

  info.freemem = getFreeMemory();
  info.nproc = getMyProcNum();
  if (copyout(p->pagetable, addr, (char *)(&info), sizeof(info))< 0)
  {
    return -1;
  }
  return 0;
}
```

以上是sys_info的基本内容，非常容易理解，然后就是完成剩下的获取信息的两个函数
def.h
```c
...
uint64 getFreeMemory();
//vm.c

...
uint64 getMyProcNum();
//proc.c
```

获取当前未使用的进程数非常简单，只需要遍历proc数组，然后判断state是否为UNUSED即可
proc.c
```c
uint64 getMyProcNum()
{
  struct proc *p;
  int ans = 0;
  for (int i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    if (p->state!=UNUSED)
      ans++;
  }
  return ans;
}
```

获取当前未使用的内存空间大小，这个需要看一下kalloc.c中的代码，可以看到，kalloc.c中有一个freelist的链表，这个链表中存储了所有的空闲内存块，因此只需要遍历这个链表，然后将每个空闲内存块的大小相加即可
kalloc.c
```c
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
```

观察可知，所有的空闲内存都被化为一个个run的struct，以链表的形式存在，并且链接在freelist后面，因此只需要遍历这个链表即可
kalloc.c
```c
uint64 
getFreeMemory()
{
  struct run* p = kmem.freelist;
  uint64 num = 0;
  while (p)
  {
    num ++;
    p = p->next;
  }
  return num * PGSIZE;
}
```
---
# 笔记 lab2 syscall

利用git来实现文件系统的切换
十分的方便
```shell
  $ git fetch
  $ git checkout syscall
  $ make clean
```

阅读user/trace.c

### 动态链接与静态链接的区别：

动态链接和静态链接是将库文件与应用程序链接在一起的两种方式。它们的区别在于库文件是在运行时加载还是在编译时链接。

在 Windows 系统中，静态链接将库的目标代码（.lib 文件）复制到可执行文件中，因此可执行文件变得较大。而动态链接将库的目标代码（.dll 文件）保留在磁盘上，并在应用程序启动时动态加载到内存中。因此，动态链接可以减小可执行文件的大小，并节省系统资源。

在 Unix 系统中，静态链接将库的目标代码（.a 文件）复制到可执行文件中。而动态链接将库的目标代码（.so 文件）保留在磁盘上，并在应用程序启动时动态加载到内存中。

CMake 是一个跨平台的构建工具，可以用于创建和管理项目。以下是在 CMake 中创建动态链接和静态链接的方法：

静态链接：

```cmake                      
add_library(mylib STATIC mylib.cpp)

add_executable(myapp main.cpp)

target_link_libraries(myapp PRIVATE mylib)
```
在这个例子中，add_library 命令用于创建一个名为 mylib 的静态库，add_executable 命令用于创建一个名为 myapp 的可执行文件，target_link_libraries 命令用于将 mylib 链接到 myapp 中。在编译 myapp 时，mylib 的代码将被复制到可执行文件中。

动态链接：

```cmake
add_library(mylib SHARED mylib.cpp)

add_executable(myapp main.cpp)

target_link_libraries(myapp PRIVATE mylib)
```
在这个例子中，`add_library` 命令用于创建一个名为 `mylib` 的动态库（.dll 或 .so 文件），`add_executable` 命令用于创建一个名为 myapp 的可执行文件，`target_link_libraries` 命令用于将 mylib 链接到 myapp 中。在运行 myapp 时，mylib 的代码将被动态加载到内存中。

需要注意的是，在不同的操作系统和编译器下，动态链接和静态链接的使用方法可能会有所不同。




要搜集一个文件夹下所有的 .lib 文件并将它们描述为一个变量，您可以使用 CMake 的 file(GLOB) 命令。这个命令可以在指定的目录中查找符合给定模式的文件，并将它们保存在一个变量中。

以下是一个示例，假设您想在 lib 目录中查找所有的 .lib 文件并将它们描述为一个变量 LIBS：

```cmake
# 搜集 lib 目录下所有的 .lib 文件
file(GLOB LIBS "lib/*.lib")

# 输出变量
message("Found libraries: ${LIBS}")
```
在这个例子中，file(GLOB) 命令将 lib 目录中所有以 .lib 结尾的文件保存在 LIBS 变量中。您可以在 message 命令中使用这个变量来输出所有找到的库文件的列表。请注意，这个命令可能会包含一些非预期的文件，例如 .lib~ 或 .lib.old 等备份文件，因此您应该检查和过滤这些文件。

gdb:
-exec file user/_ls

---