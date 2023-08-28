# lab5 COW: Copy-on-Write Fork for xv6 实验报告

**The problem**  
The fork() system call in xv6 copies all of the parent process's user-space memory into the child. If the parent is large, copying can take a long time. Worse, the work is often largely wasted; for example, a fork() followed by exec() in the child will cause the child to discard the copied memory, probably without ever using most of it. On the other hand, if both parent and child use a page, and one or both writes it, a copy is truly needed.

**The solution**  
The goal of copy-on-write (COW) fork() is to defer allocating and copying physical memory pages for the child until the copies are actually needed, if ever.
COW fork() creates just a pagetable for the child, with PTEs for user memory pointing to the parent's physical pages. COW fork() marks all the user PTEs in both parent and child as not writable. When either process tries to write one of these COW pages, the CPU will force a page fault. The kernel page-fault handler detects this case, allocates a page of physical memory for the faulting process, copies the original page into the new page, and modifies the relevant PTE in the faulting process to refer to the new page, this time with the PTE marked writeable. When the page fault handler returns, the user process will be able to write its copy of the page.

COW fork() makes freeing of the physical pages that implement user memory a little trickier. A given physical page may be referred to by multiple processes' page tables, and should be freed only when the last reference disappears.


**问题**  
在xv6中，fork()系统调用会复制父进程的所有用户空间内存到子进程中。如果父进程很大，复制可能需要很长时间。更糟糕的是，这项工作通常是浪费的;例如，fork()后跟exec()会导致子进程丢弃复制的内存，而且可能从来不会使用其中大部分。


**解决方案**  
copy-on-write（COW）fork()的目标是推迟为子进程分配和复制物理内存页，直到实际需要，如果有的话。
COW fork()仅为子进程创建一个页表，其中包含指向父进程物理页的用户内存的PTE。 COW fork()将父进程和子进程中的所有用户PTE标记为不可写。当任一进程尝试写入这些COW页中的一个时，CPU会强制执行页面错误。内核页面错误处理程序检测到此情况后，为故障进程分配一个物理内存页，将原始页面复制到新页面，并修改故障进程中的相关PTE，以引用新页面，此时将PTE标记为可写。当页面错误处理程序返回时，用户进程将能够写入其页面副本。

COW fork()使释放实现用户内存的物理页面变得更加棘手。给定的物理页面可能由多个进程的页面表引用，并且只有在最后一个引用消失时才应该释放。

## 1.Implement copy-on write(hard)

**1.1实验要求（英文）：**  
Your task is to implement copy-on-write fork in the xv6 kernel. You are done if your modified kernel executes both the cowtest and usertests programs successfully.
To help you test your implementation, we've provided an xv6 program called cowtest (source in user/cowtest.c). cowtest runs various tests, but even the first will fail on unmodified xv6. Thus, initially, you will see:
```
$ cowtest
simple: fork() failed
$ 
```
The "simple" test allocates more than half of available physical memory, and then fork()s. The fork fails because there is not enough free physical memory to give the child a complete copy of the parent's memory.
When you are done, your kernel should pass all the tests in both cowtest and usertests. That is:
```
$ cowtest
simple: ok
simple: ok
three: zombie!
ok
three: zombie!
ok
three: zombie!
ok
file: ok
ALL COW TESTS PASSED
$ usertests
...
ALL TESTS PASSED
$
```
Here's a reasonable plan of attack.

1. Modify uvmcopy() to map the parent's physical pages into the child, instead of allocating new pages. Clear PTE_W in the PTEs of both child and parent.
2. Modify usertrap() to recognize page faults. When a page-fault occurs on a COW page, allocate a new page with kalloc(), copy the old page to the new page, and install the new page in the PTE with PTE_W set.
3. Ensure that each physical page is freed when the last PTE reference to it goes away -- but not before. A good way to do this is to keep, for each physical page, a "reference count" of the number of user page tables that refer to that page. Set a page's reference count to one when kalloc() allocates it. Increment a page's reference count when fork causes a child to share the page, and decrement a page's count each time any process drops the page from its page table. kfree() should only place a page back on the free list if its reference count is zero. It's OK to to keep these counts in a fixed-size array of integers. You'll have to work out a scheme for how to index the array and how to choose its size. For example, you could index the array with the page's physical address divided by 4096, and give the array a number of elements equal to highest physical address of any page placed on the free list by kinit() in kalloc.c.
4. Modify *copyout()* to use the same scheme as page faults when it encounters a *COW* page.

Some hints:

+ The lazy page allocation lab has likely made you familiar with much of the xv6 kernel code that's relevant for copy-on-write. However, you should not base this lab on your lazy allocation solution; instead, please start with a fresh copy of xv6 as directed above.
+ It may be useful to have a way to record, for each PTE, whether it is a *COW* mapping. You can use the RSW (reserved for software) bits in the RISC-V PTE for this.
+ *usertests* explores scenarios that *cowtest* does not test, so don't forget to check that all tests pass for both.
+ Some helpful macros and definitions for page table flags are at the end of *kernel/riscv.h*.
+ If a *COW* page fault occurs and there's no free memory, the process should be killed.


**1.2实验要求（中文）：**  
你的任务是在xv6内核中实现写时复制fork。如果修改后的内核可以成功执行cowtest和usertests程序，则完成。

为了帮助您测试实现，我们提供了一个名为cowtest的xv6程序（源代码在user / cowtest.c中）。 cowtest运行各种测试，但即使是第一个测试在未修改的xv6上也会失败。因此，最初，您将看到：
```
$ cowtest
simple: fork() failed
$ 
```
"simple"测试分配了超过一半的可用物理内存，然后fork（）。fork失败，因为没有足够的空闲物理内存来为子进程提供父进程内存的完整副本。
完成后，您的内核应通过cowtest和usertests中的所有测试。也就是说： 
```
$ cowtest
simple: ok
simple: ok
three: zombie!
ok
three: zombie!
ok
three: zombie!
ok
file: ok
ALL COW TESTS PASSED
$ usertests
...
ALL TESTS PASSED
$
```

这是一个合理的“攻坚”计划。

1. 修改uvmcopy（）以将父页面的物理页面映射到子页面，而不是分配新页面。清除父子两个页面的PTE_W。
2. 修改usertrap（）以识别页面错误。当COW页面发生页面错误时，使用kalloc（）分配新页面，将旧页面复制到新页面，并使用PTE_W将新页面安装在PTE中。
3. 确保每个物理页面在最后一个PTE引用它消失之前被释放 - 但不是之前。这样做的好方法是，对于每个物理页面，保持每个用户页面表对该页面的引用数“引用计数”。当kalloc（）分配它时，将页面的引用计数设置为1。当fork导致子进程共享页面时，将页面的引用计数递增，并且每当任何进程从其页面表中删除页面时，都会将页面的引用计数递减。如果页面的引用计数为零，则kfree（）应仅将页面放回空闲列表。在固定大小的整数数组中保留这些计数是可以的。您必须解决如何索引数组以及如何选择其大小的方案。例如，您可以使用页面的物理地址除以4096来索引数组，并在kalloc.c中使用kinit（）放置在空闲列表上的任何页面的最高物理地址的元素数。
4. 修改*copyout（）*以在遇到*COW*页面时使用与页面错误相同的方案。

一些提示：

+ 懒惰的页面分配实验室可能使您熟悉与写时复制相关的xv6内核代码的大部分内容。但是，您不应该基于您的懒惰分配解决方案来完成此实验室；相反，请按照上面的说明从头开始使用xv6。
+ 为了记录每个PTE是否是COW映射，可能有用的是有一个方法。您可以使用RISC-V PTE中的RSW（保留给软件）位来完成此操作。
+ usertests探索cowtest没有测试的场景，因此不要忘记检查所有测试是否都通过。
+ 有关页面表标志的一些有用的宏和定义位于kernel/riscv.h的末尾。
+ 如果发生COW页面错误并且没有空闲内存，则应杀死该进程。

**1.3实验思路与代码**  

1. 首先是针对底层逻辑的一个分析，我们在fork时不再申请新的物理内存，而是直接讲页表映射到原本的物理内存的位置，然后将原本的物理内存页增加引用计数。

2. 对于这些物理页我们首先就要有一个计数系统，准备一个巨大的数组，数组中每个数字都对应了一个物理页的引用计数，根据物理页的形式，只需要将物理地址除以4096（也可以写作pa>>12) 即可得到对应的数组下标，然后将对应的数字加一即可。
而且设计相应的查询引用次数的函数并修改free函数，当引用次数为0时才能释放物理页。

以上这套对于物理内存系统引用次数结构的建立是本实验的关键

3. 然后是对于物理页，当fork后的未访问过的物理页应该设计为不可写，只可读，当发生写操作是会产生pagefault，为了检测是否为cow引起的页不可写，所以要给页增加一个标志位（使用保留标识位）
riscv.h
```c
#define PTE_COW (1L << 8) //CopyOnWrite
```
当trap检测到是cow问题时进行trap的逻辑判断

4. trap的逻辑判断，trap是典型的page_fault处理部分，写异常的页错误通常为0xd和0xe,在该判断下，进行cow判断和处理。
若标识位PTE_COW为1，那么检查该页的ref，若>1,则取消页的map，申请一块新的权限完整的物理页，将原页的内容拷贝过去，将地址映射到新的物理页上去。
若ref==1,则直接将标识位PTE_COW置0，将PTE_W置1，即可。

所以本实验关键关注四部分
proc    kalloc   pte   trap

根据实验为我们提供的“攻坚”计划，首先我们先阅读fork中所调用的uvmcopy
`proc.c`
```c
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  pte_t *newpte;
  uint64 pa, i;
  uint flags;
  //char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    // if((mem = kalloc()) == 0)
    //   goto err;
    // memmove(mem, (char*)pa, PGSIZE);
    // if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
    //   kfree(mem);
    //   goto err;
    // }
    *pte &= ~(PTE_W);
    *pte |= (PTE_COW);
    flags = PTE_FLAGS(*pte);
    if (mappages(new, i, PGSIZE, pa, flags) != 0)
    {
      //kfree(mem);
      goto err;
    }
    //如何为pa计数？准备一个超大的数组？
    krefalloc((char *)pa);
    if ((newpte = walk(new, i, 0)) == 0)
      panic("uvmcopy: newpte walk failed");
  }
  return 0;
  
 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}
```

`kalloc.c`
```c
#define refIndex(pa) (pa-KERNBASE)/PGSIZE
struct page_ref{
  struct spinlock lock;
  int reftimes;
};

struct page_ref page_ref[(PHYSTOP - KERNBASE) / PGSIZE];

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  for (int i = 0; i < (PHYSTOP - KERNBASE) / PGSIZE;i++){
    initlock(&(page_ref[i].lock), "page_ref");
  }
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    page_ref[refIndex((uint64)p)].reftimes = 1;
    kfree(p);
  }
    
}

void
kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run*)pa;
  //printf("the pa:%p will be free,reftimes is %d\n", (uint64)pa, r->reftimes);
  acquire(&(page_ref[refIndex((uint64)pa)].lock));
  page_ref[refIndex((uint64)pa)].reftimes--;
  if (page_ref[refIndex((uint64)pa)].reftimes < 1)
  {
    // Fill with junk to catch dangling refs.
    //printf("free the page:%p\n", (uint64)pa);
    memset(pa, 1, PGSIZE);
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&(page_ref[refIndex((uint64)pa)].lock));
}

void *
kalloc(void)
{
  struct run *r;
  uint64 pa;

  acquire(&kmem.lock);
  r = kmem.freelist;
  pa = (uint64)r;

  //此处有大坑，一定要记住将acquire放在if里面，否则在测试超内存情况下时会出现acquire找不到锁的情况，这个地方被坑了好久
  if (r){
    uint64 i=refIndex(pa);
    acquire(&page_ref[i].lock);
    page_ref[refIndex(pa)].reftimes = 1;
    release(&page_ref[i].lock);
    kmem.freelist = r->next;
  }
  release(&kmem.lock);
  if (r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


//用于增加引用计数
void
krefalloc(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kRef Alloc");

  acquire(&(page_ref[refIndex((uint64)pa)].lock));
  page_ref[refIndex((uint64)pa)].reftimes++;
  release(&(page_ref[refIndex((uint64)pa)].lock));
}

//用于减少引用计数
void
krefdel(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kRef Alloc");

  acquire(&(page_ref[refIndex((uint64)pa)].lock));
  page_ref[refIndex((uint64)pa)].reftimes--;
  release(&(page_ref[refIndex((uint64)pa)].lock));
}

//用于查询引用计数
int
get_kreftimes(void* pa){
  int ans;
  acquire(&(page_ref[refIndex((uint64)pa)].lock));
  ans = page_ref[refIndex((uint64)pa)].reftimes;
  release(&(page_ref[refIndex((uint64)pa)].lock));
  return ans;
}
```

`trap.c`
```c
void
usertrap(void)
{
    ...
    else if(r_scause()==13 || r_scause()==15){
        uint64 va = r_stval();
        if(iscow(va,p->pagetable)<0)
        p->killed = 1;
        else
        {
        if(reWriteVa(va,p->pagetable)<0)
            p->killed = 1;
        }
    }
    ...
}
```

`vm.c`
```c
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  ...
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    else{
      uint64 pa = PTE2PA(*pte);
      if(get_kreftimes((void*)pa)>1)
        krefdel((void*)pa);
    }
  ...
}
//要处理当没有调用free时，引用计数也要随着mmap减少，但要防止被误删的情况（只减少引用计数，不删除内存块


int reWriteVa(uint64 va,pagetable_t pagetable){
  pte_t *pte;
  pte_t *newpte;
  char *oldpa;
  char *newpa;
  uint flags;

  pte = walk(pagetable, va, 0);
  oldpa = (char *)PTE2PA(*pte);
  // printf("write Page Fault %p \n", va);
  // printf("the page is user Page:%d \n", *pte & (PTE_U));

  // 如果只有一个索引，并且因为cow不能阅读，那么就恢复他的阅读权限即可
  if(get_kreftimes(oldpa)==1){
    *pte |= PTE_W;
    *pte &= ~(PTE_COW);
    return 0;
  }

  //否则需要分配新的内存
  newpa = kalloc();
  if(newpa==0)
    return -1;
  memmove(newpa, oldpa, PGSIZE);
  flags = PTE_FLAGS(*pte);

  *pte = PA2PTE((uint64)newpa) | flags | PTE_W;

  //uvmunmap(pagetable, PGROUNDDOWN(va), 1, 0);
  // if (mappages(pagetable, PGROUNDDOWN(va), PGSIZE, (uint64)newpa,flags|PTE_W )<0){
  //   uvmunmap(pagetable, PGROUNDDOWN(va), 1, 0);
  //   kfree(newpa);
  //   return -1;
  // }
  newpte = walk(pagetable, va, 0);
  *newpte &= ~PTE_COW;
  kfree((void *)oldpa);
  return 0;
}
```

以上为基础需要注意的事项，除此之外，还应当注意内核对于用户物理页的访问
像copyout中就有对用户空间的内存进行访问的情况，这时候也需要进行cow的处理  
`vm.c`
```c
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  if(iscow(dstva,pagetable)==0){
    if(reWriteVa(dstva, pagetable)<0)
      return -1;
  }

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    void *mappa = (void *)(pa0 + (dstva - va0));
    //在memmove之前就要恢复可写状态
    memmove(mappa, src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
  
}
```



---
# 笔记

### 1.从硬件角度解释计算机的中断机制

中断机制是计算机硬件实现的重要特性，它允许计算机在执行程序时，能够及时响应硬件设备的请求，保证设备与计算机的协同工作。

下面是计算机硬件实现中断机制的基本过程：

1. 硬件设备向 CPU 发送中断请求信号，CPU 确认中断请求，并暂停当前正在执行的程序，将当前的指令和寄存器的值保存到堆栈中。

2. CPU 转到中断服务程序，并且将控制权交给中断服务程序，中断服务程序会根据中断类型进行相应的操作。

3. 中断服务程序完成后，将之前保存在堆栈中的指令和寄存器的值恢复，CPU 重新开始执行之前被中断的程序。

下面是更详细的解释：

1. 硬件设备向 CPU 发送中断请求信号
   
当硬件设备需要 CPU 的处理时，它会向 CPU 发送一个中断请求信号，信号可能是一个电平信号或一个脉冲信号。这个信号会被连接到 CPU 的中断输入端口。

2. CPU 确认中断请求，并暂停当前正在执行的程序
   
当 CPU 接收到中断请求信号时，它会确认中断请求并暂停当前正在执行的程序。暂停后，CPU 将当前的指令和寄存器的值保存到堆栈中。保存现场的目的是为了在处理完中断服务程序后能够恢复到之前被中断的状态。

3. CPU 转到中断服务程序
   
当 CPU 暂停程序后，它会转到中断服务程序的入口地址，并将控制权交给中断服务程序。中断服务程序是一个特殊的程序，用于处理各种不同类型的中断请求，例如定时器中断、键盘中断等。

4. 中断服务程序进行相应的操作

中断服务程序会根据中断类型进行相应的操作，例如处理输入设备的数据、响应用户输入等。在处理完中断请求后，中断服务程序会将处理结果保存在寄存器或存储器中。

5. 恢复现场

中断服务程序完成后，将之前保存在堆栈中的指令和寄存器的值恢复，CPU 重新开始执行之前被中断的程序。

6. 继续执行原来的程序

CPU 会从堆栈中恢复中断前的现场，继续执行原来的程序，并且保证从之前被中断的地方继续执行。

---
### 2.硬件中断的特殊之处

但是中断又有一些不一样的地方，这就是为什么我们要花一节课的时间来讲它。中断与系统调用主要有3个小的差别：
+ `asynchronous`。当硬件生成中断时，Interrupt handler与当前运行的进程在CPU上没有任何关联。但如果是系统调用的话，系统调用发生在运行进程的context下。
+ `concurrency`。我们这节课会稍微介绍并发，在下一节课，我们会介绍更多并发相关的内容。对于中断来说，CPU和生成中断的设备是并行的在运行。网卡自己独立的处理来自网络的packet，然后在某个时间点产生中断，但是同时，CPU也在运行。所以我们在CPU和设备之间是真正的并行的，我们必须管理这里的并行。
+ `program device`。我们这节课主要关注外部设备，例如网卡，UART，而这些设备需要被编程。每个设备都有一个编程手册，就像RISC-V有一个包含了指令和寄存器的手册一样。设备的编程手册包含了它有什么样的寄存器，它能执行什么样的操作，在读写控制寄存器的时候，设备会如何响应。不过通常来说，设备的手册不如RISC-V的手册清晰，这会使得对于设备的编程会更加复杂。


pagefault 的scause是15