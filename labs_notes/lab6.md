# Lab 6 Multithreading 实验报告

## 1. Uthread: switching between threads (moderate)

### **1.1实验要求（英文）：**  
In this exercise you will design the context switch mechanism for a user-level threading system, and then implement it. To get you started, your xv6 has two files *user/uthread.c* and user/uthread_switch.S, and a rule in the Makefile to build a uthread program. *uthread.c* contains most of a user-level threading package, and code for three simple test threads. The threading package is missing some of the code to create a thread and to switch between threads.

Your job is to come up with a plan to create threads and save/restore registers to switch between threads, and implement that plan. When you're done, make grade should say that your solution passes the uthread test.

Once you've finished, you should see the following output when you run uthread on xv6 (the three threads might start in a different order):
```sh
$ make qemu
...
$ uthread
thread_a started
thread_b started
thread_c started
thread_c 0
thread_a 0
thread_b 0
thread_c 1
thread_a 1
thread_b 1
...
thread_c 99
thread_a 99
thread_b 99
thread_c: exit after 100
thread_a: exit after 100
thread_b: exit after 100
thread_schedule: no runnable threads
$
```
This output comes from the three test threads, each of which has a loop that prints a line and then yields the CPU to the other threads.

At this point, however, with no context switch code, you'll see no output.

You will need to add code to thread_create() and thread_schedule() in user/uthread.c, and thread_switch in user/uthread_switch.S. One goal is ensure that when thread_schedule() runs a given thread for the first time, the thread executes the function passed to thread_create(), on its own stack. Another goal is to ensure that thread_switch saves the registers of the thread being switched away from, restores the registers of the thread being switched to, and returns to the point in the latter thread's instructions where it last left off. You will have to decide where to save/restore registers; modifying struct thread to hold registers is a good plan. You'll need to add a call to thread_switch in thread_schedule; you can pass whatever arguments you need to thread_switch, but the intent is to switch from thread t to next_thread.

Some hints:

+ thread_switch needs to save/restore only the callee-save registers. Why?
+ You can see the assembly code for uthread in user/uthread.asm, which may be handy for debugging.
+ To test your code it might be helpful to single step through your thread_switch using riscv64-linux-gnu-gdb. You can get started in this way:
```
(gdb) file user/_uthread
Reading symbols from user/_uthread...
(gdb) b uthread.c:60
```
This sets a breakpoint at line 60 of uthread.c. The breakpoint may (or may not) be triggered before you even run uthread. How could that happen?

Once your xv6 shell runs, type "uthread", and gdb will break at line 60. Now you can type commands like the following to inspect the state of uthread:
```
  (gdb) p/x *next_thread
```
With "x", you can examine the content of a memory location:
```
  (gdb) x/x next_thread->stack
```
You can skip to the start of thread_switch thus:
```
   (gdb) b thread_switch
   (gdb) c
```
You can single step assembly instructions using:
```
   (gdb) si
```
On-line documentation for gdb is here.

### **1.2实验要求（中文）：**  
在这个练习中，您将为用户级线程系统设计上下文切换机制，然后实现它。为了让您开始工作，您的xv6有两个文件user/uthread.c和user/uthread_switch.S，以及一个规则在Makefile中构建uthread程序。uthread.c包含大部分用户级线程包的代码，并包含三个简单测试线程的代码。线程包缺少一些用于创建线程和在线程之间切换的代码。

你的工作是想出一个计划来创建线程，并保存/恢复寄存器以在线程之间切换，并实现该计划。完成后，make grade应该说您的解决方案通过了uthread测试。

完成后，在xv6上运行uthread时，您应该会看到以下输出（三个线程的启动顺序可能会有所不同）：

```sh
$ make qemu
...
$ uthread
thread_a started
thread_b started
thread_c started
thread_c 0
thread_a 0
thread_b 0
thread_c 1
thread_a 1
thread_b 1
...
thread_c 99
thread_a 99
thread_b 99
thread_c: exit after 100
thread_a: exit after 100
thread_b: exit after 100
thread_schedule: no runnable threads
$
```
这个输出来自三个测试线程，每个线程都有一个循环，打印一行，然后将CPU让给其他线程。

但是，现在，没有context切换代码，你将看不到输出。

您需要在*user/uthread.c*中的 *thread_create()* 和 *thread_schedule()* 以及 *user/uthread_switch.S* 中的 *thread_switch* 中添加代码。一个目标是确保当*thread_schedule()*第一次运行给定线程时，线程在自己的堆栈上执行传递给*thread_create()*的函数。另一个目标是确保*thread_switch*保存被切换线程的寄存器，恢复被切换到线程的寄存器，并返回到后者的指令中的最后一次离开的点。您必须决定在哪里保存/恢复寄存器;修改 *struct thread* 以保存寄存器是一个好主意。您需要在 *thread_schedule* 中添加对 *thread_switch* 的调用;您可以将您需要的任何参数传递给 *thread_switch* ，但意图是从线程t切换到 *next_thread*。

一些提示：

+ thread_switch只需要保存/恢复被调用者保存的寄存器。为什么？
+ 您可以在user / uthread.asm中看到uthread的汇编代码，这可能对调试有用。
+ 要测试代码，使用riscv64-linux-gnu-gdb单步执行thread_switch可能会很有用。您可以按照以下方式开始：
```
(gdb) file user/_uthread
Reading symbols from user/_uthread...
(gdb) b uthread.c:60
```
这在uthread.c的第60行设置了断点。断点在运行uthread之前（或之后）可能会（或可能不会）触发。这可能发生吗？

一旦你的xv6 shell运行，输入“uthread”，gdb将在第60行中断。现在，您可以键入以下命令来检查uthread的状态：
```
  (gdb) p/x *next_thread
```
使用“x”可以检查内存位置的内容：
```
  (gdb) x/x next_thread->stack
```
您可以跳到thread_switch的开始：
```
   (gdb) b thread_switch
   (gdb) c
```
您可以使用以下命令逐条执行汇编指令：
```
   (gdb) si
```
gdb的在线文档在此处。

### **1.3实验思路与代码**
首先要阅读一下的user/uthread.c中的代码，知道这个用户线程切换是要干什么

```c
struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};
struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(uint64, uint64);
              
void 
thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void 
thread_schedule(void)
{
  struct thread *t, *next_thread;

  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  for(int i = 0; i < MAX_THREAD; i++){
    if(t >= all_thread + MAX_THREAD)
      t = all_thread;
    if(t->state == RUNNABLE) {
      next_thread = t;
      break;
    }
    t = t + 1;
  }

  if (next_thread == 0) {
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }

  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->state = RUNNABLE;
  // YOUR CODE HERE
}

void 
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void 
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int 
main(int argc, char *argv[]) 
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  thread_schedule();
  exit(0);
}
```

从提供的代码可以知道基本情况，通过用户函数创建uthread，在三个函数中调用thread_yield()函数，然后在thread_schedule()函数中进行线程切换，这里的线程切换是通过汇编实现的.

我们首先要明确，一个线程的切换，在context中需要哪些内容，核心的两个要保存的寄存器就是sp和ra，sp是指向栈顶的，ra是指当前函数运行结束后应该返回到哪里去。在线程调度中，当前函数就是thread_switch,而ra就是要跳转到的进程

context中需要保存的内容我们参考xv6的context的寄存器
```c
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};
```

为了简化，我们就不自己建一个context了，将context中的内容直接放到thread里面了
`user/uthread.c`
```c
struct thread {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;

  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};
```

在该文件中，我们规定了用户线程的最大数量
```c
struct thread all_thread[MAX_THREAD];
```
用一个静态数组来保存他

以上就是基本的分析，对于线程的创建，我们需要做的是将分配的struct thread中的ra设置为要运行的函数的地址，sp设置为栈顶(因为栈是从上往下扩张的，所以栈顶就是栈的最后一个元素的地址)
```c
void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->state = RUNNABLE;
  // YOUR CODE HERE
  t->ra = (uint64)func;
  t->sp = (uint64)&(t->stack[STACK_SIZE - 1]);
}
```

关键是涉及对于线程的切换，首先是找到一个runable的别的线程，然后调用switch，将当前线程切换为新的线程即可
```c
void 
thread_schedule(void)
{
    ...
  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
    thread_switch((uint64)t, (uint64)current_thread);
    //将当前进程重新设置为调度进程
    //current_thread = &all_thread[0];
    //注意，由于该程序并没有一个单独的调度线程，所以不需要把当前线程再次设置为thread[0]
  }
  else
    next_thread = 0;
}
```

对于线程切换的汇编代码，可以直接用swtch.S的代码
switch其实也很简单，就分两步，首先是存当前的寄存器的内容（ra和sp），然后是加载下一个线程的内容到寄存器中
`uthread_switch.S`
```c
thread_switch:
	/* YOUR CODE HERE */
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd s0, 16(a0)
	sd s1, 24(a0)
	sd s2, 32(a0)
	sd s3, 40(a0)
	sd s4, 48(a0)
	sd s5, 56(a0)
	sd s6, 64(a0)
	sd s7, 72(a0)
	sd s8, 80(a0)
	sd s9, 88(a0)
	sd s10, 96(a0)
	sd s11, 104(a0)

	ld ra, 0(a1)
	ld sp, 8(a1)
	ld s0, 16(a1)
	ld s1, 24(a1)
	ld s2, 32(a1)
	ld s3, 40(a1)
	ld s4, 48(a1)
	ld s5, 56(a1)
	ld s6, 64(a1)
	ld s7, 72(a1)
	ld s8, 80(a1)
	ld s9, 88(a1)
	ld s10, 96(a1)
	ld s11, 104(a1)
	ret    /* return to ra */
```

## 2.Using threads (moderate)  
### **2.1 实验要求（英文）：**  
In this assignment you will explore parallel programming with threads and locks using a hash table. You should do this assignment on a real Linux or MacOS computer (not xv6, not qemu) that has multiple cores. Most recent laptops have multicore processors.

This assignment uses the UNIX pthread threading library. You can find information about it from the manual page, with man pthreads, and you can look on the web, for example here, here, and here.

The file notxv6/ph.c contains a simple hash table that is correct if used from a single thread, but incorrect when used from multiple threads. In your main xv6 directory (perhaps ~/xv6-labs-2021), type this:
```sh
$ make ph
$ ./ph 1
```
Note that to build ph the Makefile uses your OS's gcc, not the 6.S081 tools. The argument to ph specifies the number of threads that execute put and get operations on the the hash table. After running for a little while, ph 1 will produce output similar to this:
```sh
100000 puts, 3.991 seconds, 25056 puts/second
0: 0 keys missing
100000 gets, 3.981 seconds, 25118 gets/second
```
The numbers you see may differ from this sample output by a factor of two or more, depending on how fast your computer is, whether it has multiple cores, and whether it's busy doing other things.

ph runs two benchmarks. First it adds lots of keys to the hash table by calling put(), and prints the achieved rate in puts per second. The it fetches keys from the hash table with get(). It prints the number keys that should have been in the hash table as a result of the puts but are missing (zero in this case), and it prints the number of gets per second it achieved.

You can tell ph to use its hash table from multiple threads at the same time by giving it an argument greater than one. Try ph 2:
```sh
$ ./ph 2
100000 puts, 1.885 seconds, 53044 puts/second
1: 16579 keys missing
0: 16579 keys missing
200000 gets, 4.322 seconds, 46274 gets/second
```
The first line of this ph 2 output indicates that when two threads concurrently add entries to the hash table, they achieve a total rate of 53,044 inserts per second. That's about twice the rate of the single thread from running ph 1. That's an excellent "parallel speedup" of about 2x, as much as one could possibly hope for (i.e. twice as many cores yielding twice as much work per unit time).
However, the two lines saying 16579 keys missing indicate that a large number of keys that should have been in the hash table are not there. That is, the puts were supposed to add those keys to the hash table, but something went wrong. Have a look at notxv6/ph.c, particularly at put() and insert().

Why are there missing keys with 2 threads, but not with 1 thread? Identify a sequence of events with 2 threads that can lead to a key being missing. Submit your sequence with a short explanation in answers-thread.txt
To avoid this sequence of events, insert lock and unlock statements in put and get in notxv6/ph.c so that the number of keys missing is always 0 with two threads. The relevant pthread calls are:
```sh
pthread_mutex_t lock;            // declare a lock
pthread_mutex_init(&lock, NULL); // initialize the lock
pthread_mutex_lock(&lock);       // acquire lock
pthread_mutex_unlock(&lock);     // release lock
```
You're done when make grade says that your code passes the ph_safe test, which requires zero missing keys with two threads. It's OK at this point to fail the ph_fast test.

Don't forget to call pthread_mutex_init(). Test your code first with 1 thread, then test it with 2 threads. Is it correct (i.e. have you eliminated missing keys?)? Does the two-threaded version achieve parallel speedup (i.e. more total work per unit time) relative to the single-threaded version?

There are situations where concurrent put()s have no overlap in the memory they read or write in the hash table, and thus don't need a lock to protect against each other. Can you change ph.c to take advantage of such situations to obtain parallel speedup for some put()s? Hint: how about a lock per hash bucket?

Modify your code so that some put operations run in parallel while maintaining correctness. You're done when make grade says your code passes both the ph_safe and ph_fast tests. The ph_fast test requires that two threads yield at least 1.25 times as many puts/second as one thread.

### **2.2 实验要求（英文）：**  

在这个作业中，您将使用线程和锁来探索并行编程，使用哈希表。您应该在具有多个内核的真实Linux或MacOS计算机（不是xv6，不是qemu）上完成此作业。最新的笔记本电脑都有多核处理器。

此作业使用UNIX pthread线程库。您可以从手册页中找到有关它的信息，其中包含man pthread和您可以在网络上查找的信息，例如这里，这里和这里。

文件notxv6 / ph.c包含一个简单的哈希表，如果从单个线程中使用，则是正确的，但是在从多个线程中使用时是不正确的。在您的主xv6目录中（可能是〜/ xv6-labs-2021），键入：
```sh
$ make ph
$ ./ph 1
```
以下是将选中的内容翻译成的中文
注意，要构建ph，Makefile使用您的OS的gcc，而不是6.S081工具。ph的参数指定在哈希表上执行put和get操作的线程数。运行了一段时间后，ph 1将产生类似于以下内容的输出：
```sh
100000 puts, 3.991 seconds, 25056 puts/second
0: 0 keys missing
100000 gets, 3.981 seconds, 25118 gets/second
```
 为什么有两个线程的时候会丢失key，但是一个线程的时候不会呢？确定一个两个线程的事件序列可以导致丢失key。在answers-thread.txt中提交您的序列和简短的解释
为了避免这种事件序列，在notxv6/ph.c中的put和get中插入lock和unlock语句，以便使用两个线程时丢失的key始终为0。相关的pthread调用是：
```sh
pthread_mutex_t lock;            // declare a lock
pthread_mutex_init(&lock, NULL); // initialize the lock
pthread_mutex_lock(&lock);       // acquire lock
pthread_mutex_unlock(&lock);     // release lock
```
当make grade说你的代码通过ph_safe测试时，你就完成了，这需要两个线程都没有丢失的key。在这一点上，失败的ph_fast测试是可以的。

不要忘记调用pthread_mutex_init()。先用1个线程测试你的代码，然后用2个线程测试。它是否正确(即你是否消除了丢失的key？)？双线程版本是否相对于单线程版本实现了并行加速(即单位时间内完成更多的总工作量)？

有些情况下，并发的put()在哈希表中没有重叠的内存读写，因此不需要锁来防止彼此。你能改变ph.c以利用这种情况来获得一些put()的并行加速吗？提示：每个哈希桶一个锁？

修改你的代码，使得一些put操作在保持正确性的同时运行在并行中。当make grade说你的代码通过ph_safe和ph_fast测试时，你就完成了。ph_fast测试要求两个线程的put/秒至少比一个线程的put/秒多1.25倍。

### **1.3实验思路与代码**
首先到notxv6/ph.c中查看代码，发现代码中有两个函数，一个是put，一个是get，put函数是往哈希表中插入数据，get函数是从哈希表中获取数据，这两个函数都是在哈希表中进行操作，所以需要加锁，否则会出现数据丢失的情况。

`ph.c`
```c
struct entry {
  int key;
  int value;
  struct entry *next;
};
struct entry *table[NBUCKET];
int keys[NKEYS];
```

观察数据结构，该实验中提供的是一个哈希链表，首先是定义了entry的数据结构，这种数据结构可以作为链表。然后在table这个哈希表中存了一些链表的head，在多线程中要注意的是，多个线程共同访问同一个链表，可能会出现节点链接错误或者丢失的情况，因此要为每个链表都加锁

`ph.c`
```c
...
pthread_mutex_t key_lock[NBUCKET];
...

//增加节点
static 
void put(int key, int value)
{
  int i = key % NBUCKET;
  pthread_mutex_lock(&key_lock[i]);
  // is the key already present?
  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key)
      break;
  }
  if (e)
  {
    // update the existing key.
    e->value = value;
  }
  else
  {
    // the new is new.
    insert(key, value, &table[i], table[i]);
  }
  pthread_mutex_unlock(&key_lock[i]);
}

//获取节点
static struct entry*
get(int key)
{
  int i = key % NBUCKET;


  struct entry *e = 0;
  pthread_mutex_lock(&key_lock[i]);
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key) break;
  }
  pthread_mutex_unlock(&key_lock[i]);

  return e;
}

//在开始的时候要记得初始化锁
int
main(int argc, char *argv[])
{
  ...
  for (int i = 0; i < NKEYS; i++) {
    keys[i] = random();
  }
  for (int i = 0; i < NBUCKET;i++){
    pthread_mutex_init(&(key_lock[i]), NULL);
  }
  ...
}
```

## 3.Barrier(moderate)  
### **3.1 实验要求（英文）：**  
In this assignment you'll implement a barrier: a point in an application at which all participating threads must wait until all other participating threads reach that point too. You'll use pthread condition variables, which are a sequence coordination technique similar to xv6's sleep and wakeup.

You should do this assignment on a real computer (not xv6, not qemu).

The file notxv6/barrier.c contains a broken barrier.
```sh
$ make barrier
$ ./barrier 2
barrier: notxv6/barrier.c:42: thread: Assertion `i == t' failed.
```
The 2 specifies the number of threads that synchronize on the barrier ( nthread in barrier.c). Each thread executes a loop. In each loop iteration a thread calls barrier() and then sleeps for a random number of microseconds. The assert triggers, because one thread leaves the barrier before the other thread has reached the barrier. The desired behavior is that each thread blocks in barrier() until all nthreads of them have called barrier().
Your goal is to achieve the desired barrier behavior. In addition to the lock primitives that you have seen in the ph assignment, you will need the following new pthread primitives; look here and here for details.
```c
pthread_cond_wait(&cond, &mutex);  // go to sleep on cond, releasing lock mutex, acquiring upon wake up
pthread_cond_broadcast(&cond);     // wake up every thread sleeping on cond
```
Make sure your solution passes make grade's barrier test.

`pthread_cond_wait` releases the mutex when called, and re-acquires the mutex before returning.
We have given you barrier_init(). Your job is to implement barrier() so that the panic doesn't occur. We've defined struct barrier for you; its fields are for your use.

There are two issues that complicate your task:

+ You have to deal with a succession of barrier calls, each of which we'll call a round. bstate.round records the current round. You should increment bstate.round each time all threads have reached the barrier.
+ You have to handle the case in which one thread races around the loop before the others have exited the barrier. In particular, you are re-using the bstate.nthread variable from one round to the next. Make sure that a thread that leaves the barrier and races around the loop doesn't increase bstate.nthread while a previous round is still using it.

Test your code with one, two, and more than two threads.

### **3.2 实验要求（中文）：**  
在这个作业中，您将实现一个屏障：应用程序中的一个点，在该点参与的所有线程都必须等待，直到所有其他参与的线程也到达该点。您将使用pthread条件变量，这是一种类似于xv6的sleep和wakeup的序列协调技术。

您应该在真实计算机上完成此作业（而不是xv6，也不是qemu）。

文件notxv6 / barrier.c包含了一个损坏的屏障。
```sh
$ make barrier
$ ./barrier 2
barrier: notxv6/barrier.c:42: thread: Assertion `i == t' failed.
```

2 指定了在 barrier 上同步的线程数（barrier.c 中的 nthread）。每个线程执行一个循环。在每个循环迭代中，线程调用 barrier() 然后睡眠随机微秒数。触发断言，因为一个线程在另一个线程到达栅栏之前离开了栅栏。期望的行为是，每个线程在 barrier() 中阻塞，直到所有 nthreads 调用 barrier()。
您的目标是实现所需的屏障行为。除了您在 ph 任务中看到的锁原语之外，您还需要以下新的 pthread 原语。这里和这里查看详情。

```c
pthread_cond_wait(&cond, &mutex);  // go to sleep on cond, releasing lock mutex, acquiring upon wake up
pthread_cond_broadcast(&cond);     // wake up every thread sleeping on cond
```

确保你的解决方案通过了make grade的障碍测试。
当调用时，`pthread_cond_wait`会释放互斥锁，并在返回之前重新获取互斥锁。
我们已经为您提供了`barrier_init()`。你的工作就是实现`barrier()`，这样就不会发生恐慌。我们为您定义了`struct barrier`，它的字段供您使用。

有两个问题会使你的任务复杂化：

+ 你必须处理一系列的障碍调用，我们称之为一轮。`bstate.round`记录了当前的轮次。每次所有线程都到达障碍时，你应该递增`bstate.round`。
+ 你必须处理一个线程在其他线程退出障碍之前就在循环中赛跑的情况。特别是，你正在从一轮到另一轮重复使用`bstate.nthread`变量。确保一个离开障碍并在循环中赛跑的线程，不会在前一轮仍在使用它时增加`bstate.nthread`。

使用一个、两个和两个以上的线程测试你的代码。

### **3.3 实验思路：**

面对这个实验，我们首先还是阅读提供的barria.c文件，了解其大致的结构和实现。  
```c
struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

for (i = 0; i < 20000; i++) {
int t = bstate.round;
assert (i == t);
barrier();
usleep(random() % 100);
}
```
设计了一个barrier的结构体，要求所有线程在同一轮次都调用一个barrier函数后才能继续往下走，要求设计barrier函数。
根据题目要求，barrier函数就可以大致设计出来，没个线程调用barrier时，bstate的nthread就+1，若nthread未到达要求数量，线程进入等待（wait），否则将round+1将thread置为0之后，唤醒所有线程（broadcast）。

`barrier.c`
```c
static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);
  bstate.nthread++;
  if (bstate.nthread<nthread)
  {
    pthread_cond_wait(&bstate.barrier_cond,&bstate.barrier_mutex);
  }
  else{
    bstate.round++;
    bstate.nthread = 0;
    pthread_cond_broadcast(&bstate.barrier_cond);
  }
  pthread_mutex_unlock(&bstate.barrier_mutex);
}
```