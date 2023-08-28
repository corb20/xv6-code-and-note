# lab8 lock: Parallelism 实验报告

## 1. Memory allocator (moderate)

### **1.1 实验要求（英文）：**
The program user/kalloctest stresses xv6's memory allocator: three processes grow and shrink their address spaces, resulting in many calls to kalloc and kfree. kalloc and kfree obtain kmem.lock. kalloctest prints (as "#fetch-and-add") the number of loop iterations in acquire due to attempts to acquire a lock that another core already holds, for the kmem lock and a few other locks. The number of loop iterations in acquire is a rough measure of lock contention. The output of kalloctest looks similar to this before you complete the lab:

```sh
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 83375 #acquire() 433015
lock: bcache: #fetch-and-add 0 #acquire() 1260
--- top 5 contended locks:
lock: kmem: #fetch-and-add 83375 #acquire() 433015
lock: proc: #fetch-and-add 23737 #acquire() 130718
lock: virtio_disk: #fetch-and-add 11159 #acquire() 114
lock: proc: #fetch-and-add 5937 #acquire() 130786
lock: proc: #fetch-and-add 4080 #acquire() 130786
tot= 83375
test1 FAIL
```
acquire maintains, for each lock, the count of calls to acquire for that lock, and the number of times the loop in acquire tried but failed to set the lock. kalloctest calls a system call that causes the kernel to print those counts for the kmem and bcache locks (which are the focus of this lab) and for the 5 most contended locks. If there is lock contention the number of acquire loop iterations will be large. The system call returns the sum of the number of loop iterations for the kmem and bcache locks.

For this lab, you must use a dedicated unloaded machine with multiple cores. If you use a machine that is doing other things, the counts that kalloctest prints will be nonsense. You can use a dedicated Athena workstation, or your own laptop, but don't use a dialup machine.

The root cause of lock contention in kalloctest is that kalloc() has a single free list, protected by a single lock. To remove lock contention, you will have to redesign the memory allocator to avoid a single lock and list. The basic idea is to maintain a free list per CPU, each list with its own lock. Allocations and frees on different CPUs can run in parallel, because each CPU will operate on a different list. The main challenge will be to deal with the case in which one CPU's free list is empty, but another CPU's list has free memory; in that case, the one CPU must "steal" part of the other CPU's free list. Stealing may introduce lock contention, but that will hopefully be infrequent.

Your job is to implement per-CPU freelists, and stealing when a CPU's free list is empty. You must give all of your locks names that start with "kmem". That is, you should call initlock for each of your locks, and pass a name that starts with "kmem". Run kalloctest to see if your implementation has reduced lock contention. To check that it can still allocate all of memory, run usertests sbrkmuch. Your output will look similar to that shown below, with much-reduced contention in total on kmem locks, although the specific numbers will differ. Make sure all tests in usertests pass. make grade should say that the kalloctests pass.

```sh
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 42843
lock: kmem: #fetch-and-add 0 #acquire() 198674
lock: kmem: #fetch-and-add 0 #acquire() 191534
lock: bcache: #fetch-and-add 0 #acquire() 1242
--- top 5 contended locks:
lock: proc: #fetch-and-add 43861 #acquire() 117281
lock: virtio_disk: #fetch-and-add 5347 #acquire() 114
lock: proc: #fetch-and-add 4856 #acquire() 117312
lock: proc: #fetch-and-add 4168 #acquire() 117316
lock: proc: #fetch-and-add 2797 #acquire() 117266
tot= 0
test1 OK
start test2
total free number of pages: 32499 (out of 32768)
.....
test2 OK
$ usertests sbrkmuch
usertests starting
test sbrkmuch: OK
ALL TESTS PASSED
$ usertests
...
ALL TESTS PASSED
$
```

Some hints:

+ You can use the constant NCPU from kernel/param.h
+ Let freerange give all free memory to the CPU running freerange.
+ The function cpuid returns the current core number, but it's only safe to call it and use its result when interrupts are turned off. You should use push_off() and pop_off() to turn interrupts off and on.
+ Have a look at the snprintf function in kernel/sprintf.c for string formatting ideas. It is OK to just name all locks "kmem" though.

### **1.2 实验要求（中文）：**

程序用户/kalloctest用于压力测试xv6的内存分配器：三个进程增长和缩小他们的地址空间，导致对kalloc和kfree的多次调用。kalloc和kfree获取kmem.lock。kalloctest打印（标记为“#fetch-and-add”）尝试获取已被另一核心持有的锁所产生的acquire循环迭代次数，这对于kmem锁以及其他一些锁都是如此。acquire中的循环迭代次数是锁竞争的粗略度量。在你完成实验室之前，kalloctest的输出可能看起来类似于这样：

 ```sh
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 83375 #acquire() 433015
lock: bcache: #fetch-and-add 0 #acquire() 1260
--- top 5 contended locks:
lock: kmem: #fetch-and-add 83375 #acquire() 433015
lock: proc: #fetch-and-add 23737 #acquire() 130718
lock: virtio_disk: #fetch-and-add 11159 #acquire() 114
lock: proc: #fetch-and-add 5937 #acquire() 130786
lock: proc: #fetch-and-add 4080 #acquire() 130786
tot= 83375
test1 FAIL
```
 acquire为每个锁维护了acquire调用次数的计数，以及acquire循环尝试设置该锁的失败次数的计数。kalloctest调用了一个系统调用，导致内核打印出kmem和bcache锁的这些计数（这是本实验的重点），以及5个最有竞争的锁的计数。如果有锁竞争，则acquire循环迭代的次数将会很大。系统调用返回kmem和bcache锁的循环迭代次数之和。

在本实验中，你必须使用一个专用的未加载内核的机器。如果你使用正在做其他事情的机器，kalloctest打印的计数将是无意义的。你可以使用专用的Athena工作站，或者你自己的笔记本电脑，但不要使用拨号机。

kalloctest中锁竞争的根本原因是kalloc()有一个由单一锁保护的空闲列表。为了消除锁竞争，你将需要重新设计内存分配器，避免使用单一锁和列表。基本的想法是为每个CPU维护一个空闲列表，每个列表都有自己的锁。不同CPU上的分配和释放可以并行运行，因为每个CPU将操作不同的列表。主要的挑战将是处理当一个CPU的空闲列表为空，但另一个CPU的列表有空闲内存的情况；在这种情况下，一个CPU必须“窃取”另一个CPU空闲列表的一部分。窃取可能会引入锁竞争，但我们希望这种情况是不常见的。

你的任务是实现每CPU的空闲列表，以及当CPU的空闲列表为空时的窃取。你必须给所有的锁起名字，这些名字应该以“kmem”开头。也就是说，你应该为每个锁调用initlock，并传入一个以“kmem”开头的名字。运行kalloctest来看看你的实现是否减少了锁竞争。为了检查它是否仍然可以分配所有的内存，运行usertests sbrkmuch。你的输出会看起来类似于下面所示的，尽管具体的数字会有所不同，但在kmem锁上的总体竞争已大大减少。确保所有在usertests中的测试都通过。make grade应该会显示kalloctests已经通过。

```sh
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 42843
lock: kmem: #fetch-and-add 0 #acquire() 198674
lock: kmem: #fetch-and-add 0 #acquire() 191534
lock: bcache: #fetch-and-add 0 #acquire() 1242
--- top 5 contended locks:
lock: proc: #fetch-and-add 43861 #acquire() 117281
lock: virtio_disk: #fetch-and-add 5347 #acquire() 114
lock: proc: #fetch-and-add 4856 #acquire() 117312
lock: proc: #fetch-and-add 4168 #acquire() 117316
lock: proc: #fetch-and-add 2797 #acquire() 117266
tot= 0
test1 OK
start test2
total free number of pages: 32499 (out of 32768)
.....
test2 OK
$ usertests sbrkmuch
usertests starting
test sbrkmuch: OK
ALL TESTS PASSED
$ usertests
...
ALL TESTS PASSED
$
```

一些提示：

+ 你可以使用 kernel/param.h 中的常量 NCPU
+ 让 freerange 将所有空闲内存分配给运行 freerange 的 CPU
+ 函数 cpuid 返回当前内核编号，但是只有在关闭中断的时候才可以安全地调用它和使用它的返回值。你应该使用 push_off() 和 pop_off() 来关闭和打开中断。
+ 查看 kernel/sprintf.c 中的 snprintf 函数以获得字符串格式化的思路。将所有锁都命名为 "kmem" 也是可以的。

### **1.3 实验思路与代码：**

该实验要做什么已经很明确了。
但要完成该实验还是要好好理解kalloc是如何工作的，对于内存中的地址空间，我们最开始就把他们写成一个一个struct run的形式，以链表的形式存储，

下图是freelist的示意图，每个struct run都是一个空闲的内存块，每个struct run都有一个next指针，指向下一个空闲的内存块，这样就形成了一个链表。

而分配块就是每次从链表的顶端取一个块,将freelist指向next即可，分配走的块就随便装什么东西都可以。

```
freelist->  +-------------------+
            |     struct run    |--------+
             -------------------         |
            |                   |        | next
            |                   |        |
            +-------------------+  <-----+
            |     struct run    |--------+
             -------------------         |
            |                   |        |
            |                   |        |
            +-------------------+  <-----+
            |     struct run    |--------+
             -------------------         |
            |                   |        |
            |                   |        |
            +-------------------+  <-----+
            |     struct run    |--------+
             -------------------         |
            |                   |        |
            |                   |        |
            +-------------------+  <-----+
            |     struct run    |--------+
             -------------------         |
            |                   |        |
            |                   |        |
            +-------------------+  <-----+
```

据此我们在设计的时候就可以直接讲原本的地址空间，从KERNEL_BASE到PTYSTOP，分成NCPU份，每个CPU都有自己的freelist，这样就可以避免锁竞争。

这是大体的思路
`kalloc.c`
```c
struct kmem {
  struct spinlock lock;
  struct run *freelist;
} ;

struct kmem nkmem[NCPU];


void
kinit()
{
  CMemSize = (PHYSTOP - (uint64)end) / NCPU;
  for (int nki = 0; nki < NCPU;nki++){
    char kmem_name[8];
    snprintf(kmem_name, 6, "kmem_%d", nki);
    initlock(&nkmem[nki].lock, kmem_name);
  }
  freerange(end, (void *)PHYSTOP);
}

//在free这件事情上，还都是还到自己对应的freelist上
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  // Fill with junk to catch dangling refs.

  memset(pa, 1, PGSIZE);
  r = (struct run*)pa;
  //获取cpu编号
  int ci=((uint64)pa-(uint64)end)/CMemSize;
  acquire(&nkmem[ci].lock);
  r->next = nkmem[ci].freelist;
  nkmem[ci].freelist = r;
  release(&nkmem[ci].lock);
}

//分配时先是获取自己的cpuid，在自己的对应区块去取内存，分配不到了再去别人的区块抢
void *
kalloc(void)
{
  struct run *r;

  int ci;

  push_off();
  ci= cpuid();
  pop_off();

  acquire(&nkmem[ci].lock);
  r = nkmem[ci].freelist;
  if(r) 
    nkmem[ci].freelist = r->next;
  release(&nkmem[ci].lock);

  int cio=ci+1;

  //此处还会涉及到抢占问题(去别人的分区抢page空间，完成的非常的nice)
  while (!r && cio!=ci)
  {
    acquire(&nkmem[cio].lock);
    r = nkmem[cio].freelist;
    if(r) 
      nkmem[cio].freelist = r->next;
    release(&nkmem[cio].lock);
    cio=(cio+1)%NCPU;
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
```

## 2. Buffer cache (hard)

### **2.1 实验要求（英文）：**

This half of the assignment is independent from the first half; you can work on this half (and pass the tests) whether or not you have completed the first half.

If multiple processes use the file system intensively, they will likely contend for bcache.lock, which protects the disk block cache in kernel/bio.c. bcachetest creates several processes that repeatedly read different files in order to generate contention on bcache.lock; its output looks like this (before you complete this lab):

```sh
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 33035
lock: bcache: #fetch-and-add 16142 #acquire() 65978
--- top 5 contended locks:
lock: virtio_disk: #fetch-and-add 162870 #acquire() 1188
lock: proc: #fetch-and-add 51936 #acquire() 73732
lock: bcache: #fetch-and-add 16142 #acquire() 65978
lock: uart: #fetch-and-add 7505 #acquire() 117
lock: proc: #fetch-and-add 6937 #acquire() 73420
tot= 16142
test0: FAIL
start test1
test1 OK
```
You will likely see different output, but the number of acquire loop iterations for the bcache lock will be high. If you look at the code in kernel/bio.c, you'll see that bcache.lock protects the list of cached block buffers, the reference count (b->refcnt) in each block buffer, and the identities of the cached blocks (b->dev and b->blockno).
Modify the block cache so that the number of acquire loop iterations for all locks in the bcache is close to zero when running bcachetest. Ideally the sum of the counts for all locks involved in the block cache should be zero, but it's OK if the sum is less than 500. Modify bget and brelse so that concurrent lookups and releases for different blocks that are in the bcache are unlikely to conflict on locks (e.g., don't all have to wait for bcache.lock). You must maintain the invariant that at most one copy of each block is cached. When you are done, your output should be similar to that shown below (though not identical). Make sure usertests still passes. make grade should pass all tests when you are done.
```sh
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 32954
lock: kmem: #fetch-and-add 0 #acquire() 75
lock: kmem: #fetch-and-add 0 #acquire() 73
lock: bcache: #fetch-and-add 0 #acquire() 85
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4159
lock: bcache.bucket: #fetch-and-add 0 #acquire() 2118
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4274
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4326
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6334
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6321
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6704
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6696
lock: bcache.bucket: #fetch-and-add 0 #acquire() 7757
lock: bcache.bucket: #fetch-and-add 0 #acquire() 6199
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4136
lock: bcache.bucket: #fetch-and-add 0 #acquire() 4136
lock: bcache.bucket: #fetch-and-add 0 #acquire() 2123
--- top 5 contended locks:
lock: virtio_disk: #fetch-and-add 158235 #acquire() 1193
lock: proc: #fetch-and-add 117563 #acquire() 3708493
lock: proc: #fetch-and-add 65921 #acquire() 3710254
lock: proc: #fetch-and-add 44090 #acquire() 3708607
lock: proc: #fetch-and-add 43252 #acquire() 3708521
tot= 128
test0: OK
start test1
test1 OK
$ usertests
  ...
ALL TESTS PASSED
$
```
Please give all of your locks names that start with "bcache". That is, you should call initlock for each of your locks, and pass a name that starts with "bcache".

Reducing contention in the block cache is more tricky than for kalloc, because bcache buffers are truly shared among processes (and thus CPUs). For kalloc, one could eliminate most contention by giving each CPU its own allocator; that won't work for the block cache. We suggest you look up block numbers in the cache with a hash table that has a lock per hash bucket.

There are some circumstances in which it's OK if your solution has lock conflicts:

+ When two processes concurrently use the same block number. bcachetest test0 doesn't ever do this.
+ When two processes concurrently miss in the cache, and need to find an unused block to replace. bcachetest test0 doesn't ever do this.
+ When two processes concurrently use blocks that conflict in whatever scheme you use to partition the blocks and locks; for example, if two processes use blocks whose block numbers hash to the same slot in a hash table. bcachetest test0 might do this, depending on your design, but you should try to adjust your scheme's details to avoid conflicts (e.g., change the size of your hash table).
bcachetest's test1 uses more distinct blocks than there are buffers, and exercises lots of file system code paths.

Here are some hints:

+ Read the description of the block cache in the xv6 book (Section 8.1-8.3).
+ It is OK to use a fixed number of buckets and not resize the hash table dynamically. Use a prime number of buckets (e.g., 13) to reduce the likelihood of hashing conflicts.
+ Searching in the hash table for a buffer and allocating an entry for that buffer when the buffer is not found must be atomic.
+ Remove the list of all buffers (bcache.head etc.) and instead time-stamp buffers using the time of their last use (i.e., using ticks in kernel/trap.c). With this change brelse doesn't need to acquire the bcache lock, and bget can select the least-recently used block based on the time-stamps.
+ It is OK to serialize eviction in bget (i.e., the part of bget that selects a buffer to re-use when a lookup misses in the cache).
+ Your solution might need to hold two locks in some cases; for example, during eviction you may need to hold the bcache lock and a lock per bucket. Make sure you avoid deadlock.
+ When replacing a block, you might move a struct buf from one bucket to another bucket, because the new block hashes to a different bucket. You might have a tricky case: the new block might hash to the same bucket as the old block. Make sure you avoid deadlock in that case.
+ Some debugging tips: implement bucket locks but leave the global bcache.lock acquire/release at the beginning/end of bget to serialize the code. Once you are sure it is correct without race conditions, remove the global locks and deal with concurrency issues. You can also run make CPUS=1 qemu to test with one core.

### **2.1 实验要求（中文）：**

这个任务的后半部分与前半部分无关；您可以在完成前半部分之前或之后，独立地完成这个任务（并通过测试）。

如果多个进程密集地使用文件系统，它们可能会争夺bcache.lock，该锁保护kernel/bio.c中的磁盘块缓存。bcachetest创建了几个进程，重复读取不同的文件以在bcache.lock上产生争用；在您完成本实验之前，它的输出看起来像这样：
```sh
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 0 #acquire() 33035
lock: bcache: #fetch-and-add 16142 #acquire() 65978
--- top 5 contended locks:
lock: virtio_disk: #fetch-and-add 162870 #acquire() 1188
lock: proc: #fetch-and-add 51936 #acquire() 73732
lock: bcache: #fetch-and-add 16142 #acquire() 65978
lock: uart: #fetch-and-add 7505 #acquire() 117
lock: proc: #fetch-and-add 6937 #acquire() 73420
tot= 16142
test0: FAIL
start test1
test1 OK
```

请给你的所有锁命名以“bcache”开头。也就是说，您应该为每个锁调用initlock，并传递以“bcache”开头的名称。

减少块缓存中的争用比kalloc更棘手，因为bcache缓冲区确实在进程（因此CPU）之间共享。对于kalloc，可以通过为每个CPU提供自己的分配器来消除大部分争用；这对于块缓存是行不通的。我们建议您使用每个哈希桶一个锁的哈希表来查找缓存中的块号。

有些情况下，如果你的解决方案有锁冲突也是可以接受的 :
+ 当两个进程同时使用相同的块号时。bcachetest test0永远不会这样做。
+ 当两个进程同时在缓存中缺失，并且需要找到一个未使用的块来替换时。bcachetest test0永远不会这样做。
+ 当两个进程同时使用在分区块和锁中冲突的块时，例如，如果两个进程使用块，其块号在哈希表中的同一槽中哈希。bcachetest test0可能会这样做，这取决于您的设计，但是您应该尝试调整方案的细节以避免冲突（例如，更改哈希表的大小）。
bcachetest的test1使用的块比缓冲区多，并且可以执行许多文件系统代码路径。

一些提示：

+ 请阅读 xv6 书中对 block cache 的描述 (Section 8.1-8.3)。
+ 用固定数目的 buckets 而不是动态调整 hash table 的大小是可以接受的。使用一个质数的 buckets (e.g., 13) 以降低哈希冲突的可能性。
+ 在哈希表中搜索 buffer 并为其分配一个 entry 时，必须是原子性的。
+ 去除所有 buffers 的链表 (bcache.head 等)，而是使用其最后使用的时间戳（例如，使用 kernel/trap.c 中的 ticks）来标记 buffer。通过这个变化，brelse 不需要获取 bcache lock，而且 bget 可以根据时间戳选择最近未使用的 block。
+ 在 bget 中，替换 block 时可能需要同时持有两个锁；例如，在替换时可能需要持有 bcache lock 和 bucket lock。请确保避免死锁。
+ 当替换 block 时，可能需要移动一个 struct buf 到另一个 bucket，因为新的 block 的哈希值与旧的 block 不同。你可能会遇到一个复杂的情况：新的 block 与旧的 block 可能哈希到相同的 bucket。请确保在这种情况下避免死锁。
+ 一些调试技巧：实现 bucket locks，但在 bget 的开始/结束处保留全局的 bcache.lock 来序列化代码。一旦你确定没有竞态条件，就删除全局的锁并处理并发问题。你也可以运行 make CPUS=1 qemu 来使用一个核心测试。

### **2.1 实验思路与代码：**

根据要求与提示，我们使用一个哈希链表的形式来存储block，每个哈希链表都有一个head，block们依次挂在哈希链表的head上面，锁只需要针对哈希数组中的每个值来设置，而不是为所有的block只设置一个锁，这样可以有效减少冲突

哈希值则是针对blockno来取的

`param.h`
```c
#define BUCKSIZE     13 //桶的数量
```

`bio.c`
```c
//这是设计的哈希链表
struct {
  struct spinlock lock;
  struct buf head;
}bufbuck[BUCKSIZE];

int hash(int x){
  return x%BUCKSIZE;
}

//将之前所命名的块，根据自己的序号，全部挂载在哈希链表上
void
binit(void)
{
  struct buf *b;
  int bki;
  char bkname[16];
  //int nmsize;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  
  for(bki=0;bki<BUCKSIZE;bki++){
    bufbuck[bki].head.prev = &bufbuck[bki].head;
    bufbuck[bki].head.next = &bufbuck[bki].head;
    snprintf(bkname,11,"bcachebk%d\0",bki);
    initlock(&(bufbuck[bki].lock),bkname);
  }

  bki=0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    b->next=bufbuck[bki].head.next;
    b->prev=&(bufbuck[bki].head);
    initsleeplock(&b->lock,"buffer");
    bufbuck[bki].head.next->prev=b;
    bufbuck[bki].head.next=b;

    bki=(bki+1)%BUCKSIZE;
  }
}

//基本逻辑很简单，先根据blockno获取哈希值，在自己对应的哈希桶内去取，若已经空了，那么去别的桶内抢，具体从哪个桶抢，此处还可以加入时间戳优化（但是我懒了
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bki;
  int bkio;

  bki=hash(blockno);

  acquire(&bufbuck[bki].lock);

  // Is the block already cached?
  for(b = bufbuck[bki].head.next; b != &bufbuck[bki].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bufbuck[bki].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bufbuck[bki].head.prev; b != &bufbuck[bki].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bufbuck[bki].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  
  //如果自己的桶下面没有就去别的桶下面抢(记得将拓扑关系改过来)
  bkio=bki+1;
  while(bkio!=bki)
  {
    acquire(&bufbuck[bkio].lock);
    for(b = bufbuck[bkio].head.prev; b != &bufbuck[bkio].head; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        //将拓扑关系挪过来
        b->prev->next=b->next;
        b->next->prev=b->prev;

        b->next=bufbuck[bki].head.next;
        b->prev=&(bufbuck[bki].head);
        bufbuck[bki].head.next->prev=b;
        bufbuck[bki].head.next=b;

        release(&bufbuck[bki].lock);
        release(&bufbuck[bkio].lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bufbuck[bkio].lock);
    bkio=(bkio+1)%BUCKSIZE;
  }
  
  panic("bget: no buffers");
}

void
brelse(struct buf *b)
{
  //int bki;
  int bki_now;

  //bki=(int)(b-bcache.buf)/(sizeof(struct buf));
  bki_now=hash(b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bufbuck[bki_now].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //首先还原原本的链表关系
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    
    // b->next = bufbuck[bki].head.next;
    // b->prev = &bufbuck[bki].head;
    // bufbuck[bki].head.next->prev = b;
    // bufbuck[bki].head.next = b;
  }
  
  release(&bufbuck[bki_now].lock);
}

```