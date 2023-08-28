# lab3 pgtbl 实验报告

## 1.Speed up system calls (easy)

```实验要求（英文）：```

Some operating systems (e.g., Linux) speed up certain system calls by sharing data in a read-only region between userspace and the kernel. This eliminates the need for kernel crossings when performing these system calls. To help you learn how to insert mappings into a page table, your first task is to implement this optimization for the getpid() system call in xv6.

When each process is created, map one read-only page at USYSCALL (a VA defined in memlayout.h). At the start of this page, store a struct usyscall (also defined in memlayout.h), and initialize it to store the PID of the current process. For this lab, ugetpid() has been provided on the userspace side and will automatically use the USYSCALL mapping. You will receive full credit for this part of the lab if the ugetpid test case passes when running pgtbltest.

Some hints:

+ You can perform the mapping in proc_pagetable() in kernel/proc.c.
+ Choose permission bits that allow userspace to only read the page.
+ You may find that mappages() is a useful utility.
+ Don't forget to allocate and initialize the page in allocproc().
+ Make sure to free the page in freeproc().

```实验要求（中文）：```

操作系统(例如, Linux)通过在用户空间和内核之间共享只读区域中的数据来加快某些系统调用的速度. 这消除了执行这些系统调用时内核穿越的需要. 为了帮助您学习如何将映射插入页表, 您的第一个任务是为xv6中的getpid()系统调用实现此优化. 

当创建每个进程时，在USYSCALL（memlayout.h中定义的VA）处映射一个只读页面。在此页面的开头，存储一个struct usyscall（也在memlayout.h中定义），并将其初始化为存储当前进程的PID。对于此实验室，已在用户空间端提供ugetpid（）并将自动使用USYSCALL映射。当运行pgtbltest时，如果ugetpid测试用例通过，则将获得此实验部分的全部学分。

一些提示：
+ 在kernel/proc.c中的 *proc_pagetable()* 中执行映射。
+ 选择允许用户空间只读页面的权限位。
+ 您可能会发现 *mappages()* 是一个有用的实用程序。
+ 不要忘记在 *allocproc()* 中分配和初始化页面。
+ 确保在 *freeproc()* 中释放页面。

```实验代码：```

我们首先观察已经给出的实验代码，其中包含了 *ugetpid()* 函数，该函数用于获取当前进程的进程号。

```c
int
ugetpid(void)
{
  struct usyscall *u = (struct usyscall *)USYSCALL;
  return u->pid;
}
```
```c
struct usyscall {
  int pid;  // Process ID
};
```
就是从一块用户空间中读取进程的id
这块用户空间在内核中被映射向一块申请出的内存中，可以仿照 `TRAPFRAME` 来理解

在```proc.h```中记录一个usyscall的指针
```c
struct proc {
  struct spinlock lock;
  ...
  struct usyscall *usyscall;
};
```

在allocproc()中申请一块内存，并将usyscall指向这块内存
```proc.c```
```c
static struct proc*
allocproc(void)
{
  struct proc *p;
  ...
found:
  ...
  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  //Allocate a usyscall page.(因为对于usyscall来说只申请了指针，并没有真的申请存储内容的物理页块，所以要先分配物理页块)
  //kalloc返回的是内核空间中可用的虚地址，在内核空间中每次使用虚地址会被默认（由硬件实现）转化为物理地址，所以可以在后面map中作为物理地址传入
  if((p->usyscall=(struct usyscall*)kalloc())==0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  ...
  p->usyscall->pid = p->pid;
  ...
}
```

在proc_pagetable()中，我们需要将这块内存映射到用户空间中，这样用户空间就可以访问这块内存了
```c
pagetable_t
proc_pagetable(struct proc *p)
{
  ...
  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  if(mappages(pagetable, USYSCALL, PGSIZE,
              (uint64)(p->usyscall), PTE_R | PTE_U) < 0){
    uvmunmap(pagetable, USYSCALL, 1, 0);
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }  
  ...
}
```

还要记得在freeproc中释放该内存空间
```c
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->usyscall)
    kfree((void*)p->usyscall);
  p->usyscall = 0;
  ...
}
```

以上ugetpid就成功实现了

## 2.Print a page table (easy)

**2.1 实验要求（英文）**：  
To help you visualize RISC-V page tables, and perhaps to aid future debugging, your second task is to write a function that prints the contents of a page table.

Define a function called vmprint(). It should take a pagetable_t argument, and print that pagetable in the format described below. Insert if(p->pid==1) vmprint(p->pagetable) in exec.c just before the return argc, to print the first process's page table. You receive full credit for this part of the lab if you pass the pte printout test of make grade.

Now when you start xv6 it should print output like this, describing the page table of the first process at the point when it has just finished exec()ing init:

```sh
page table 0x0000000087f6e000
 ..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
 .. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
 .. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000
 .. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000
 .. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000
 ..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
 .. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
 .. .. ..509: pte 0x0000000021fdd813 pa 0x0000000087f76000
 .. .. ..510: pte 0x0000000021fddc07 pa 0x0000000087f77000
 .. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
```

The first line displays the argument to vmprint. After that there is a line for each PTE, including PTEs that refer to page-table pages deeper in the tree. Each PTE line is indented by a number of " .." that indicates its depth in the tree. Each PTE line shows the PTE index in its page-table page, the pte bits, and the physical address extracted from the PTE. Don't print PTEs that are not valid. In the above example, the top-level page-table page has mappings for entries 0 and 255. The next level down for entry 0 has only index 0 mapped, and the bottom-level for that index 0 has entries 0, 1, and 2 mapped.
Your code might emit different physical addresses than those shown above. The number of entries and the virtual addresses should be the same.

Some hints:

+ You can put vmprint() in kernel/vm.c.
+ Use the macros at the end of the file kernel/riscv.h.
+ The function freewalk may be inspirational.
+ Define the prototype for vmprint in kernel/defs.h so that you can call it from exec.c.
+ Use %p in your printf calls to print out full 64-bit hex PTEs and addresses as shown in the example.

Explain the output of vmprint in terms of Fig 3-4 from the text. What does page 0 contain? What is in page 2? When running in user mode, could the process read/write the memory mapped by page 1? What does the third to last page contain?

**2.2 实验要求（中文）：**

为了帮助你可视化RISC-V页面表，并可能有助于未来的调试，你的第二个任务是写一个函数，打印页面表的内容。

定义一个名为`vmprint()`的函数。它应该采取一个pagetable_t参数，并以下面描述的格式打印该pagetable。在return argc之前，在exec.c中插入if(p->pid==1) vmprint(p->pagetable)，以打印第一个进程的页表。如果您通过make grade的pte打印测试，您将获得此部分实验的全部学分。

现在当你启动xv6时，它应该打印出像下面这样的输出，描述了第一个进程的页表，在它刚刚完成exec()ing init时:

```sh
page table 0x0000000087f6e000
 ..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
 .. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
 .. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000
 .. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000
 .. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000
 ..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
 .. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
 .. .. ..509: pte 0x0000000021fdd813 pa 0x0000000087f76000
 .. .. ..510: pte 0x0000000021fddc07 pa 0x0000000087f77000
 .. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
```
第一行显示vmprint的参数。之后的每一行都是一个PTE，包括引用树中深层页表页的PTE。每个PTE行的缩进都是对应数量的'..'，表示它在树中的深度。每个PTE行都显示它在页表页中的PTE索引、PTE位和从PTE中提取的物理地址。不要打印无效的PTE。在上面的示例中，顶层页表页有映射条目0和255。下一个级别的条目0只有索引0映射，而下面的底层条目0有条目0、1和2映射。
你的代码可能会输出与上面所示的不同的物理地址。条目数量和虚拟地址应该是一样的。

一些提示:
 + 在kernel/vm.c中，可以放置vmprint（）。
+ 使用文件kernel/riscv.h末尾的宏。
+ freewalk函数可能会激发灵感。
+ 在kernel/defs.h中定义vmprint的原型，以便您可以从exec.c调用它。
+ 使用％p在printf调用中打印完整的64位十六进制PTE和地址，如示例所示。

根据文本中的图3-4，解释vmprint的输出。页面0包含什么？页面2中有什么？在用户模式下运行时，进程可以读/写页面1的内存映射吗？倒数第三页包含什么？

**2.3实验思路与实验代码:**

首先根据题目要求，在exec中插入检测pid是否为1，如果为1，则输出页表的代码
```c
int
exec(char *path, char **argv)
{
  ...
  proc_freepagetable(oldpagetable, oldsz);

  if(p->pid==1)
    vmprint(p->pagetable);

  return argc; // this ends up in a0, the first argument to main(argc, argv)
  ...
}
```
在vm.c中完成相应vmprint的代码,实现思路参考walk，就是利用一个递归，逐级阅读列表，打印出来，注意要打印合法的pte，并且要明确level
```c
void ptePrint(pagetable_t, int);

void vmprint(pagetable_t pagetable)
{
  printf("page table %p\n", pagetable);
  ptePrint(pagetable, 2);
  return;
}

void ptePrint(pagetable_t pagetable,int lev)
{
  // there are 2^9 = 512 PTEs in a page table.
  char preName[3][16] = {".. .. ..", ".. ..", ".."};
  for (int i = 0; i < 512; i++)
  {
    pte_t pte = pagetable[i];
    if(pte & PTE_V){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);//打印合法的pte，并且要明确level
      printf("%s%d: pte %p pa %p\n", preName[lev], i, pte, child);
      if(lev>0)
        ptePrint((pagetable_t)child, lev - 1);
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  return;
}
```
这是一个树形的结构，完全遍历如此看来递归是最好的方法（深度优先算法dfs）

## 3.Detecting which pages have been accessed (hard)

**3.1实验要求（英文）：**

Some garbage collectors (a form of automatic memory management) can benefit from information about which pages have been accessed (read or write). In this part of the lab, you will add a new feature to xv6 that detects and reports this information to userspace by inspecting the access bits in the RISC-V page table. The RISC-V hardware page walker marks these bits in the PTE whenever it resolves a TLB miss.

Your job is to implement pgaccess(), a system call that reports which pages have been accessed. The system call takes three arguments. First, it takes the starting virtual address of the first user page to check. Second, it takes the number of pages to check. Finally, it takes a user address to a buffer to store the results into a bitmask (a datastructure that uses one bit per page and where the first page corresponds to the least significant bit). You will receive full credit for this part of the lab if the pgaccess test case passes when running pgtbltest.

Some hints:

+ Start by implementing sys_pgaccess() in kernel/sysproc.c.
+ You'll need to parse arguments using argaddr() and argint().
+ For the output bitmask, it's easier to store a temporary buffer in the kernel and copy it to the user (via copyout()) after filling it with the right bits.
+ It's okay to set an upper limit on the number of pages that can be scanned.
+ *walk()* in kernel/vm.c is very useful for finding the right PTEs.
+ You'll need to define PTE_A, the access bit, in kernel/riscv.h. Consult the RISC-V manual to determine its value.
+ Be sure to clear PTE_A after checking if it is set. Otherwise, it won't be possible to determine if the page was accessed since the last time pgaccess() was called (i.e., the bit will be set forever).
+ *vmprint()* may come in handy to debug page tables.

**3.2实验要求（中文）：**

一些garbage collector（一种自动内存管理的形式）可以从有关哪些页面已被访问（读取或写入）的信息中受益。在本实验的这一部分中，您将向xv6添加一个新功能，该功能通过检查RISC-V页表中的访问位来将此信息检测并报告给用户空间。 RISC-V硬件页行走器在解析TLB缺失时将这些位标记在PTE中。

您的工作是实现pgaccess（），该系统调用报告已访问哪些页面。系统调用有三个参数。首先，它获取要检查的第一个用户页面的起始虚拟地址。其次，它获取要检查的页面数。最后，它获取用户地址以将结果存储到位掩码中（一种数据结构，其中每个页面使用一位，其中第一个页面对应于最低有效位）。运行pgtbltest时，如果pgaccess测试用例通过，则将获得此实验的全部学分。

一些提示：

+ 从kernel/sysproc.c开始实现sys_pgaccess()。
+ 您需要使用argaddr()和argint()来解析参数。
+ 对于输出位掩码，更容易在内核中存储一个临时缓冲区，并在填充正确位之后将其复制到用户（通过copyout()）。
+ 可以在扫描的页面数量上设置上限。
+ 内核/vm.c中的*walk()*对于查找正确的PTE非常有用。
+ 您需要在kernel/riscv.h中定义PTE_A，访问位。请查阅RISC-V手册以确定其值。
+ 确保在检查PTE_A是否设置后清除它。否则，将无法确定自上次调用pgaccess()以来页面是否已被访问（即，该位将永远设置）。
+ *vmprint()*可能有助于调试页表。

**3.3实验思路与程序：**


观察pgtbltest中的测试用例，可以发现，pgaccess()函数的功能是检测给定的虚拟地址范围内的页是否被访问过，如果被访问过，则将对应的位设置为1，否则设置为0。因此，我们需要遍历给定的虚拟地址范围，对每个虚拟地址进行检测，如果被访问过，则将对应的位设置为1，否则设置为0。
```
uint64 pgaccess(void* addr, int size, int mask)
```
+ addr：虚拟地址范围的起始地址
+ size：虚拟地址范围的大小
+ mask：用于存储结果的位掩码

实现方式也很简单，利用walk访问所有的addr到addr+size*PGSIZE的虚拟地址，如果访问过，则将对应的位设置为1，否则设置为0。

但是需要记得访问之后再将PTE_A清零，否则下次访问时，会认为该页已经被访问过了。
```c
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 base;
  int len;
  uint64 maskaddr;
  struct proc *p;
  pte_t check_pte;
  int i;
  if (argaddr(0, &base) < 0 || argaddr(2, &maskaddr)<0 || argint(1, &len)<0)
    return -1;
  int mask = 0;
  p = myproc();
  // 开始搞检查
  for (i = 0;i<len;i++)
  {
    pte_t *pteptr;
    if ((pteptr = walk(p->pagetable, base + (uint64)PGSIZE*i, 0))==0)
      continue;
    check_pte=*pteptr;
    if (check_pte & PTE_A)
    {
      mask |= (1 << i);
      *pteptr ^= PTE_A;
    }
  }
  if (copyout(p->pagetable, maskaddr, (char *)&mask, sizeof(mask)) < 0)
    return -1;
  return 0;
}
```

---
# 笔记

```c
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last; // 设置虚拟地址，最后一个虚拟地址
  pte_t *pte; // 页表项指针

  if(size == 0)
    panic("mappages: size"); // 大小为空，产生致命错误

  a = PGROUNDDOWN(va); // 取虚拟地址的向下对其的地址
  last = PGROUNDDOWN(va + size - 1); // 取最后一页的向下对其的地址
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0) // 寻找页表
      return -1; // 若无法找到页表，则返回错误
    if(*pte & PTE_V)
      panic("mappages: remap"); // 如果页表项已经存在，则产生致命错误
    *pte = PA2PTE(pa) | perm | PTE_V; // 将物理地址存入页表项中，并设置相应权限和位
    if(a == last)
      break; // 逐步映射每一页
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0; // 映射完成，正常返回0
}
```
此函数用于将虚拟地址区间 `[va, va + size)` 映射到物理地址区间 `[pa, pa + size)`，并设置权限。

每次先通过向下对齐虚拟地址得到当前页的虚拟地址，再通过该虚拟地址查找页表。若页表中页表项已经存在，则表示该页已经被映射，此时需要报错。

若找到了合适的页表，则将物理地址存入页表项中，并设置相应权限和位。接着遍历下一页并继续进行映射，直至映射完成。

若执行期间出现错误，则会产生致命错误并停止程序运行。若映射成功，则函数返回 0。