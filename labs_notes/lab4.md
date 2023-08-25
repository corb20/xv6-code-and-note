# lab4 traps 实验报告

## 1.RISC-V assembly (easy)

**1.1实验要求（英文）：**  
It will be important to understand a bit of RISC-V assembly, which you were exposed to in 6.004. There is a file user/call.c in your xv6 repo. make fs.img compiles it and also produces a readable assembly version of the program in user/call.asm.

Read the code in call.asm for the functions g, f, and main. The instruction manual for RISC-V is on the reference page. Here are some questions that you should answer (store the answers in a file answers-traps.txt):

+ Which registers contain arguments to functions? For example, which register holds 13 in main's call to printf?
+ Where is the call to function f in the assembly code for main? Where is the call to g? (Hint: the compiler may inline functions.)
+ At what address is the function printf located?
+ What value is in the register ra just after the jalr to printf in main?
Run the following code.
  ```c
  unsigned int i = 0x00646c72;
  printf("H%x Wo%s", 57616, &i);
  ```
  What is the output? Here's an ASCII table that maps bytes to characters.
  The output depends on that fact that the RISC-V is little-endian. If the RISC-V were instead big-endian what would you set i to in order to yield the same output? Would you need to change 57616 to a different value?

+ In the following code, what is going to be printed after 'y='? (note: the answer is not a specific value.) Why does this happen?
```
printf("x=%d y=%d", 3);
```

**1.2实验要求（中文）:**  

 你需要理解一些RISC-V汇编语言, 你在6.004中接触过. 你的xv6 repo中有一个文件user/call.c, make fs.img编译它并且产生一个可读的程序版本在user/call.asm.

阅读call.asm中的代码, 对于函数g, f和main. RISC-V的指令手册在参考页上. 这里有一些问题你应该回答(把答案存放在answers-traps.txt文件中):

以下是解答：

+ 哪些寄存器包含函数的参数？例如，在main对printf的调用中，哪个寄存器包含13？
```
函数调用时寄存器从a0-a7依次存放参数，如果参数超过8个，多余的参数存放在栈中。
printf("%d %d\n", f(8) + 1, 13);
其中13放在a2中，"%d %d\n"放在a0中，f(8) + 1放在a1中。
```
+ 在main的汇编代码中，函数f的调用在哪里？g的调用在哪里？(提示：编译器可能会内联函数。)
```asm
void main(void) {
    ...
  printf("%d %d\n", f(8) + 1, 13);
  4a:	4635                	li	a2,13
  4c:	45b1                	li	a1,12
  4e:	00000517          	auipc	a0,0x0
  52:	7ca50513          	addi	a0,a0,1994 # 818 <malloc+0xfc>
  56:	00000097          	auipc	ra,0x0
  5a:	608080e7          	jalr	1544(ra) # 65e <printf>
    ...
  #在上述汇编中，f(8) + 1放在a1中，13放在a2中，f和g直接被内联汇编求结果，即12，不需要jalr调用。
```
+ 函数printf位于哪个地址？
```asm
000000000000065e <printf>:
```
+ 在main中对printf的jalr调用之后，寄存器ra中的值是多少？
```asm
#寄存器中ra为0x5e,即调用printf的main函数中下一条汇编的地址
void main(void) {
    ...
  5a:	608080e7          	jalr	1544(ra) # 65e <printf>
  exit(0);
  5e:	4501                	li	a0,0
    ...
```

+ 运行以下代码。
  ```c
  unsigned int i = 0x00646c72;
  printf("H%x Wo%s", 57616, &i);
  ``` 
  输出是什么？这是一个将字节映射到字符的ASCII表。
  输出取决于RISC-V是小端的这一事实。如果RISC-V是大端的，你需要将i设置为什么值才能得到相同的输出？你需要将57616更改为不同的值吗？
```
%x是指输出16进制数，57616的16进制数是e110，所以输出为He110，%s是将后续的内容以字符串的形式输出，&i是i的地址，1 int 相当于是4Byte，所以可以拆分为4个字符，即0x00,0x64,0x6c,0x72,对应的ASCII码为/0,d,l,r,所以最终输出为He110 World
```

+ 在以下代码中，在“y=”后面会打印什么？(注意：答案不是特定的值。)为什么会这样？

	printf("x=%d y=%d", 3);

```
后面会打印寄存器a2中的值，a2中的值是什么取决于之前的调用情况
```

## 2.Backtrace (moderate)
**2.1实验要求（英文）:**  
For debugging it is often useful to have a backtrace: a list of the function calls on the stack above the point at which the error occurred.

Implement a backtrace() function in kernel/printf.c. Insert a call to this function in sys_sleep, and then run bttest, which calls sys_sleep. Your output should be as follows:

```
backtrace:
0x0000000080002cda
0x0000000080002bb6
0x0000000080002898
```

After bttest exit qemu. In your terminal: the addresses may be slightly different but if you run addr2line -e kernel/kernel (or riscv64-unknown-elf-addr2line -e kernel/kernel) and cut-and-paste the above addresses as follows:  
```sh
$ addr2line -e kernel/kernel
0x0000000080002de2
0x0000000080002f4a
0x0000000080002bfc
Ctrl-D
```
You should see something like this:
```sh
kernel/sysproc.c:74
kernel/syscall.c:224
kernel/trap.c:85
```

The compiler puts in each stack frame a frame pointer that holds the address of the caller's frame pointer. Your backtrace should use these frame pointers to walk up the stack and print the saved return address in each stack frame.

Some hints:

+ Add the prototype for backtrace to kernel/defs.h so that you can invoke backtrace in sys_sleep.
The GCC compiler stores the frame pointer of the currently executing function in the register s0. Add the following function to kernel/riscv.h:  
    ```c
    static inline uint64
    r_fp()
    {
    uint64 x;
    asm volatile("mv %0, s0" : "=r" (x) );
    return x;
    }
    ```
    and call this function in backtrace to read the current frame pointer. This function uses in-line assembly to read s0.

+ These lecture notes have a picture of the layout of stack frames. Note that the return address lives at a fixed offset (-8) from the frame pointer of a stackframe, and that the saved frame pointer lives at fixed offset (-16) from the frame pointer.

+ Xv6 allocates one page for each stack in the xv6 kernel at PAGE-aligned address. You can compute the top and bottom address of the stack page by using PGROUNDDOWN(fp) and PGROUNDUP(fp) (see kernel/riscv.h. These number are helpful for backtrace to terminate its loop.  

Once your backtrace is working, call it from panic in kernel/printf.c so that you see the kernel's backtrace when it panics.

**2.2实验要求（中文）:**

调试时通常需要有一个回溯：在发生错误的地方之上的堆栈中的函数调用列表。

在kernel/printf.c中实现一个backtrace()函数。在sys_sleep中插入一个对该函数的调用，然后运行bttest，该函数调用sys_sleep。你的输出应该如下所示：

    backtrace:
    0x0000000080002cda
    0x0000000080002bb6
    0x0000000080002898

退出qemu后。在你的终端：地址可能有些不同，但如果你运行*addr2line -e kernel/kernel*（或riscv64-unknown-elf-addr2line -e kernel/kernel）并剪切和粘贴上面的地址如下所示：
```sh
$ addr2line -e kernel/kernel
0x0000000080002de2
0x0000000080002f4a
0x0000000080002bfc
Ctrl-D
```
你应该会看到这样的东西：
```sh
kernel/sysproc.c:74
kernel/syscall.c:224
kernel/trap.c:85
```
编译器在每个堆栈帧中放置一个帧指针，该帧指针保存调用者的帧指针的地址。你的回溯应该使用这些帧指针来遍历堆栈并打印每个堆栈帧中保存的返回地址。

一些提示：

+ 在kernel/defs.h中为backtrace添加原型，以便您可以在sys_sleep中调用backtrace。
GCC编译器将当前执行函数的帧指针存储在寄存器s0中。将以下函数添加到kernel/riscv.h中：
    ```c
    static inline uint64
    r_fp()
    {
    uint64 x;
    asm volatile("mv %0, s0" : "=r" (x) );
    return x;
    }
    ```
    并在backtrace中调用此函数以读取当前帧指针。此函数使用内联汇编来读取s0。

+ 这些讲座笔记有一张堆栈帧布局的图片。请注意，返回地址位于堆栈帧的帧指针的固定偏移量（-8），并且保存的帧指针位于帧指针的固定偏移量（-16）。

+ Xv6在xv6内核中为每个堆栈分配一个页面，位于页面对齐的地址。您可以通过使用PGROUNDDOWN（fp）和PGROUNDUP（fp）来计算堆栈页的顶部和底部地址（请参见kernel/riscv.h。这些数字对于backtrace终止其循环非常有用。

一旦backtrace工作，就会在kernel/printf.c中的panic中调用它，以便在发生panic时看到内核的backtrace。

**2.3实验思路与分析**

根据以上提示可以大致知道backtrace是希望我们访问内核栈，然后根据内核栈的布局，找到每个函数的返回地址，然后根据返回地址找到函数的名字。

以及寄存器的一些基本知识

| reg    | name  | saver  | description                                     |
|--------|-------|--------|-------------------------------------------------|
| x0     | zero  |        | hardwired zero                                  |
| x1     | ra    | caller | return address                                  |
| x2     | sp    | callee | stack pointer                                   |
| x3     | gp    |        | global pointer                                  |
| x4     | tp    |        | thread pointer                                  |
| x5-7   | t0-2  | caller | temporary registers                             |
| x8     | s0/fp | callee | saved register / frame pointer                  |
| x9     | s1    | callee | saved register                                  |
| x10-11 | a0-1  | caller | function arguments / return values              |
| x12-17 | a2-7  | caller | function arguments                              |
| x18-27 | s2-11 | callee | saved registers                                 |
| x28-31 | t3-6  | caller | temporary registers                             |
| pc     |       |        | program counter                                 |

回顾一下关于内核栈的基本架构。

```
Stack
                   .
                   .
                   .
      +-> +-----------------+   |
      |   | return address  |   |
      |   |   previous fp ------+
      |   | saved registers |
      |   | local variables |
      |   |       ...       | 
      |   +-----------------+ <-+
      |   | return address  |   |
      +------ previous fp   |   |
          | saved registers |   |
          | local variables |   |
          |       ...       |   |
      +-> +-----------------+   |
      |   | return address  |   |
      |   |   previous fp ------+
      |   | saved registers |
      |   | local variables |
      |   |       ...       | 
      |   +-----------------+ <-+
      |   | return address  |   |
      +------ previous fp   |   |
          | saved registers |   |
          | local variables |   |
          |       ...       |   |
  $fp --> +-----------------+   |
          | return address  |   |
          |   previous fp ------+
          | saved registers |
          | local variables |
  $sp --> +-----------------+
```

提示中还为我们提供了获取fp指针的代码
```c
static inline uint64
r_fp()
{
uint64 x;
asm volatile("mv %0, s0" : "=r" (x) );
return x;
}
```

由于xv6只为栈准备了一个页，所以可以使用PGROUNDDOWN和PGROUNDUP来计算栈的顶部和底部地址。用于终止循环。

stack中每个一条都是64bit，即8Byte，所以该函数的返回地址为fp-8，而上一个函数栈的fp的地址为fp-16。

只需要用一个while不断往回读即可

```c
void
backtrace(void)
{
  printf("backtrace:\n");
  uint64 fp = r_fp();
  uint64 bottom = PGROUNDDOWN(fp);
  uint64 top = PGROUNDUP(fp);
  while (fp>=bottom && fp<top)
  {
    uint64 ra = *(uint64 *)(fp - 8);
    printf("%p\n", ra);
    fp = *(uint64 *)(fp - 16);
  }
  return;
}
```

## 3.Alarm (hard)

**3.1实验要求（英文）：**  
In this exercise you'll add a feature to xv6 that periodically alerts a process as it uses CPU time. This might be useful for compute-bound processes that want to limit how much CPU time they chew up, or for processes that want to compute but also want to take some periodic action. More generally, you'll be implementing a primitive form of user-level interrupt/fault handlers; you could use something similar to handle page faults in the application, for example. Your solution is correct if it passes alarmtest and usertests.

You should add a new sigalarm(interval, handler) system call. If an application calls sigalarm(n, fn), then after every n "ticks" of CPU time that the program consumes, the kernel should cause application function fn to be called. When fn returns, the application should resume where it left off. A tick is a fairly arbitrary unit of time in xv6, determined by how often a hardware timer generates interrupts. If an application calls sigalarm(0, 0), the kernel should stop generating periodic alarm calls.

You'll find a file user/alarmtest.c in your xv6 repository. Add it to the Makefile. It won't compile correctly until you've added sigalarm and sigreturn system calls (see below).

alarmtest calls sigalarm(2, periodic) in test0 to ask the kernel to force a call to periodic() every 2 ticks, and then spins for a while. You can see the assembly code for alarmtest in user/alarmtest.asm, which may be handy for debugging. Your solution is correct when alarmtest produces output like this and usertests also runs correctly:

```
$ alarmtest
test0 start
........alarm!
test0 passed
test1 start
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
test1 passed
test2 start
................alarm!
test2 passed
$ usertests
...
ALL TESTS PASSED
$
```
When you're done, your solution will be only a few lines of code, but it may be tricky to get it right. We'll test your code with the version of alarmtest.c in the original repository. You can modify alarmtest.c to help you debug, but make sure the original alarmtest says that all the tests pass.

**test0: invoke handler**  
Get started by modifying the kernel to jump to the alarm handler in user space, which will cause test0 to print "alarm!". Don't worry yet what happens after the "alarm!" output; it's OK for now if your program crashes after printing "alarm!". Here are some hints:

+ You'll need to modify the Makefile to cause alarmtest.c to be compiled as an xv6 user program.
+ The right declarations to put in user/user.h are:
    int sigalarm(int ticks, void (*handler)());
    int sigreturn(void);
+ Update user/usys.pl (which generates user/usys.S), kernel/syscall.h, and kernel/syscall.c to allow alarmtest to invoke the sigalarm and sigreturn system calls.
+ For now, your sys_sigreturn should just return zero.
+ Your sys_sigalarm() should store the alarm interval and the pointer to the handler function in new fields in the proc structure (in kernel/proc.h).
+ You'll need to keep track of how many ticks have passed since the last call (or are left until the next call) to a process's alarm handler; you'll need a new field in struct proc for this too. You can initialize proc fields in allocproc() in proc.c.
+ Every tick, the hardware clock forces an interrupt, which is handled in usertrap() in kernel/trap.c.
+ You only want to manipulate a process's alarm ticks if there's a timer interrupt; you want something like
    if(which_dev == 2) ...
+ Only invoke the alarm function if the process has a timer outstanding. Note that the address of the user's alarm function might be 0 (e.g., in user/alarmtest.asm, periodic is at address 0).
+ You'll need to modify usertrap() so that when a process's alarm interval expires, the user process executes the handler function. When a trap on the RISC-V returns to user space, what determines the instruction address at which user-space code resumes execution?
+ It will be easier to look at traps with gdb if you tell qemu to use only one CPU, which you can do by running
```
    make CPUS=1 qemu-gdb
```
+ You've succeeded if alarmtest prints "alarm!".

**test1/test2(): resume interrupted code**

Chances are that alarmtest crashes in test0 or test1 after it prints "alarm!", or that alarmtest (eventually) prints "test1 failed", or that alarmtest exits without printing "test1 passed". To fix this, you must ensure that, when the alarm handler is done, control returns to the instruction at which the user program was originally interrupted by the timer interrupt. You must ensure that the register contents are restored to the values they held at the time of the interrupt, so that the user program can continue undisturbed after the alarm. Finally, you should "re-arm" the alarm counter after each time it goes off, so that the handler is called periodically.

As a starting point, we've made a design decision for you: user alarm handlers are required to call the sigreturn system call when they have finished. Have a look at *periodic* in *alarmtest.c* for an example. This means that you can add code to usertrap and sys_sigreturn that cooperate to cause the user process to resume properly after it has handled the alarm.

Some hints:

+ Your solution will require you to save and restore registers---what registers do you need to save and restore to resume the interrupted code correctly? (Hint: it will be many).
+ Have usertrap save enough state in struct proc when the timer goes off that sigreturn can correctly return to the interrupted user code.
+ Prevent re-entrant calls to the handler----if a handler hasn't returned yet, the kernel shouldn't call it again. test2 tests this.


Once you pass test0, test1, and test2 run usertests to make sure you didn't break any other parts of the kernel.

**3.2实验要求（中文）：**  

 在这个练习中，您将添加一个功能来在使用CPU时间时定期向进程发出警报。这对于想要限制自己使用CPU时间的计算绑定过程可能很有用，也对于想要计算但也想要采取一些定期操作的过程可能很有用。更普遍地说，您将实现一种原始形式的用户级中断/故障处理程序；例如，您可以使用类似的东西来处理应用程序中的页面故障。如果您的解决方案通过alarmtest和usertests，则正确。

您应该添加一个新的sigalarm（interval，handler）系统调用。如果应用程序调用sigalarm（n，fn），那么在程序消耗的每n个“刻度”之后，内核应该导致应用程序函数fn被调用。当fn返回时，应用程序应该恢复到它离开的位置。在xv6中，一个tick是一个相当任意的时间单位，它取决于硬件计时器生成中断的频率。如果应用程序调用sigalarm（0，0），内核应该停止生成周期性的警报调用。
您会在xv6存储库中找到一个文件user / alarmtest.c。将其添加到Makefile中。在添加sigalarm和sigreturn系统调用之前，它将无法正确编译（请参见下文）。

alarmtest在test0中调用sigalarm（2，periodic）要求内核每2个刻度强制调用一次periodic（），然后旋转一段时间。您可以在用户/ alarmtest.asm中看到alarmtest的汇编代码，这可能很方便进行调试。当alarmtest产生类似这样的输出时，您的解决方案就是正确的，usertests也会正确运行：
```
$ alarmtest
test0 start
........alarm!
test0 passed
test1 start
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
test1 passed
test2 start
................alarm!
test2 passed
$ usertests
...
ALL TESTS PASSED
$
```
 **测试0: 调用处理程序**  
首先修改内核，使其跳转到用户空间中的alarm处理程序，这会导致test0打印“alarm!”。不要担心“alarm!”输出后会发生什么;现在，如果您的程序在打印“alarm!”后崩溃，那没关系。以下是一些提示：

+ 您需要修改Makefile，以便将alarmtest.c编译为xv6用户程序。
+ 要放在user/user.h中的正确声明是：
    int sigalarm(int ticks, void (*handler)());
    int sigreturn(void);
+ 更新user/usys.pl(生成user/usys.S)、kernel/syscall.h和kernel/syscall.c，以允许alarmtest调用sigalarm和sigreturn系统调用。
+ 目前，您的sys_sigreturn只需返回零。
+ 您的sys_sigalarm()应该将报警间隔和处理程序的指针存储在proc结构的新字段中(在kernel/proc.h中)。
+ 您需要跟踪自上次调用进程的报警处理程序以来已经过去了多少个时钟周期(或者距离下一次调用还有多少个时钟周期);您还需要proc.c中的allocproc()中的新字段。您可以在allocproc()中的proc.c中初始化proc字段。
+ 每个时钟周期，硬件时钟都会强制发生中断，在kernel/trap.c中的usertrap()中处理。
+ 只有在有计时器中断时才要操作进程的报警时钟;您想要的东西类似于
    if(which_dev == 2) ...
+ 仅在进程拥有计时器的情况下调用报警函数。注意，用户报警函数的地址可能为0(例如，在user/alarmtest.asm中，周期位于地址0处)。
+ 您需要修改usertrap()，以便当进程的报警间隔到期时，用户进程执行处理程序函数。当RISC-V上的陷阱返回到用户空间时，什么决定了用户空间代码恢复执行的指令地址？
+ 如果告诉qemu只使用一个CPU，那么用gdb查看陷阱会更容易，您可以通过运行
```
    make CPUS=1 qemu-gdb
```
+ 如果alarmtest打印“alarm!”，则成功了。

**测试1/测试2():恢复中断的代码**

alarmtest在打印“alarm!”后崩溃在test0或test1中的可能性很大，或者alarmtest（最终）打印“test1失败”，或者alarmtest退出而不打印“test1通过”。为了修复这个问题，你必须确保当闹钟处理程序完成时，控制权返回到用户程序最初被定时器中断中断的指令。你必须确保寄存器内容被恢复到中断发生时的值，这样用户程序就可以在闹钟响起后继续不受干扰的执行。最后，在闹钟响起后，你应该“重新装载”闹钟计数器，这样处理程序就可以周期性地被调用了。

作为一个起点，我们为你做了一个设计决策:用户闹钟处理程序在完成时需要调用sigreturn系统调用。请参阅alarmtest.c中的periodic的示例。这意味着你可以添加代码到usertrap和sys_sigreturn，它们合作可以在处理完闹钟后导致用户进程正确恢复。

一些提示:

+ 你的解决方案将要求你保存和恢复寄存器——你需要保存和恢复哪些寄存器，以便正确地恢复中断的代码?（提示:它将是许多的）。
+ 当计时器响起时，让usertrap在struct proc中保存足够的状态，以便sigreturn可以正确地返回到中断的用户代码。
+ 防止对处理程序还没返回时又因为时钟中断触发siglarm，内核就不应该再次调用它。test2测试这个。

一旦你通过了test0、test1和test2，运行usertests来确保你没有破坏内核的其他部分。

**3.3实验思路与答案**

根据题目要求，可以大致确定实验的思路，在proc中添加一个interval的属性，用来记录报警间隔，默认是0。用户可以调用siglarm的系统调用对该值进行设置。

处理的方式的话，在trap中实现，如果proc中的interval不是0，那么检验计时器passed_tick是否等于interval，如果等于，则调用函数(函数存在proc中的handler属性中），否则无事发生。ps.只要发生时钟中断，passed_tick就会加一。

除了最为直观的`interval`,`passed_tick`,`handler`之外，还需要存一个`sigtrapframe`,用来存储跳转handler之前的中断信息以便与sigreturn时能够回到原位置

除此之外还需要设计一个saved_interval，来保存interval，因为我们不希望进入handler时，触发sigalarm的中断，那么需要讲interval设置为0，saved_interval用来保存原来的interval，以便于sigreturn时恢复。

在proc.h中添加相应需要的变量
```c
// Per-process state
struct proc {
  struct spinlock lock;
    ...
  struct trapframe *sigtrapframe;
  int interval;
  int saved_interval;
  int passed_tick;
  uint64 handler;
};
```

对于proc中新加的属性，在fork，allocproc以及freeproc中要注意申请和释放空间

`proc.c`
```c
static struct proc*
allocproc(void)
{
    ...
found:
    ...
  if((p->sigtrapframe=(struct trapframe*)kalloc())==0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  ...
}

static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  if(p->sigtrapframe)
    kfree((void*)p->sigtrapframe);
    ...
}
```

siglarm系统调用的实现
（实现系统调用需要的一系列前期基础工作此处省略，直接设计sys_siglarm)

`sysproc.c`
```c
//这件事本质上不是靠系统调用来完成的，而是靠时钟中断来完成的
//sigalarm只是负责将参数和将要调用的用户函数赋给进程，真实调用是在时钟中断中完成的
uint64 sys_sigalarm(void)
{
  int n;
  uint64 fn;
  if (argint(0, &n) < 0)
    return -1;
  if(argaddr(1, &fn) < 0)
    return -1;
  struct proc* p = myproc();
  p->handler = fn;
  p->interval= n;
  p->saved_interval = n;
  p->passed_tick = 0;

  return 0;
}
```
值得注意的是由于传入的函数指针在用户空间，所以当到达n个周期要执行该函数时，需要讲该函数指针赋给trapframe中的epc，这样才能在用户空间执行该函数

在`trap.c`中添加对时钟中断的处理
```c
void
usertrap(void)
{
    ...
  // give up the CPU if this is a timer interrupt.
  //该语句用于判断是否为时钟中断，若是则调用yield()函数，将当前进程放入就绪队列中
  //sigalarm很大程度上要借助这个函数实现
  if(which_dev == 2){
    p->passed_tick++;
    if (p->interval != 0 && p->passed_tick == p->interval)
    {
        //要准备一个另外的trapframe用于保存中断前的状态sigreturn
      *(p->sigtrapframe) = *(p->trapframe);
      p->passed_tick = 0;
      //此时应该回到用户空间,并且执行p->handler在用户空间所指向的函数
      p->trapframe->epc=p->handler;
      p->saved_interval = p->interval;
      p->interval = 0;//此处讲interval设为0，防止在执行用户函数时再次触发中断
    }
    yield();
  }
  ...
}
```

观察测试中被调用的函数
```c
void
periodic()
{
  count = count + 1;
  printf("alarm!\n");
  sigreturn();
}
```
函数运行完成之后必须调用sigreturn回到内核空间，然后再次回到用户空间（即发生时钟中断前的位置）
完成sigreturn的实现
`sysproc.c`
```c
uint64 sys_sigreturn(void)
{
  struct proc *p = myproc();
  *(p->trapframe) = *(p->sigtrapframe);
  p->interval = p->saved_interval;//此处可以恢复interval
  return 0;
}
```