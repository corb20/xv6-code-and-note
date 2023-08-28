# lab10 mmap实验报告

## 1.Lab: mmap (hard)

### **1.1 实验要求（英文）：**
The mmap and munmap system calls allow UNIX programs to exert detailed control over their address spaces. They can be used to share memory among processes, to map files into process address spaces, and as part of user-level page fault schemes such as the garbage-collection algorithms discussed in lecture. In this lab you'll add mmap and munmap to xv6, focusing on memory-mapped files.

The manual page (run man 2 mmap) shows this declaration for mmap:

```
void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
```
mmap can be called in many ways, but this lab requires only a subset of its features relevant to memory-mapping a file. You can assume that addr will always be zero, meaning that the kernel should decide the virtual address at which to map the file. mmap returns that address, or 0xffffffffffffffff if it fails. length is the number of bytes to map; it might not be the same as the file's length. prot indicates whether the memory should be mapped readable, writeable, and/or executable; you can assume that prot is PROT_READ or PROT_WRITE or both. flags will be either MAP_SHARED, meaning that modifications to the mapped memory should be written back to the file, or MAP_PRIVATE, meaning that they should not. You don't have to implement any other bits in flags. fd is the open file descriptor of the file to map. You can assume offset is zero (it's the starting point in the file at which to map).

It's OK if processes that map the same MAP_SHARED file do not share physical pages.

munmap(addr, length) should remove mmap mappings in the indicated address range. If the process has modified the memory and has it mapped MAP_SHARED, the modifications should first be written to the file. An munmap call might cover only a portion of an mmap-ed region, but you can assume that it will either unmap at the start, or at the end, or the whole region (but not punch a hole in the middle of a region).

You should implement enough mmap and munmap functionality to make the mmaptest test program work. If mmaptest doesn't use a mmap feature, you don't need to implement that feature.

When you're done, you should see this output:
```sh
$ mmaptest
mmap_test starting
test mmap f
test mmap f: OK
test mmap private
test mmap private: OK
test mmap read-only
test mmap read-only: OK
test mmap read/write
test mmap read/write: OK
test mmap dirty
test mmap dirty: OK
test not-mapped unmap
test not-mapped unmap: OK
test mmap two files
test mmap two files: OK
mmap_test: ALL OK
fork_test starting
fork_test OK
mmaptest: all tests succeeded
$ usertests
usertests starting
...
ALL TESTS PASSED
$ 
```
Here are some hints:

+ Start by adding _mmaptest to UPROGS, and mmap and munmap system calls, in order to get user/mmaptest.c to compile. For now, just return errors from mmap and munmap. We defined PROT_READ etc for you in kernel/fcntl.h. Run mmaptest, which will fail at the first mmap call.
+ Fill in the page table lazily, in response to page faults. That is, mmap should not allocate physical memory or read the file. Instead, do that in page fault handling code in (or called by) usertrap, as in the lazy page allocation lab. The reason to be lazy is to ensure that mmap of a large file is fast, and that mmap of a file larger than physical memory is possible.
+ Keep track of what mmap has mapped for each process. Define a structure corresponding to the VMA (virtual memory area) described in Lecture 15, recording the address, length, permissions, file, etc. for a virtual memory range created by mmap. Since the xv6 kernel doesn't have a memory allocator in the kernel, it's OK to declare a fixed-size array of VMAs and allocate from that array as needed. A size of 16 should be sufficient.
+ Implement mmap: find an unused region in the process's address space in which to map the file, and add a VMA to the process's table of mapped regions. The VMA should contain a pointer to a struct file for the file being mapped; mmap should increase the file's reference count so that the structure doesn't disappear when the file is closed (hint: see filedup). Run mmaptest: the first mmap should succeed, but the first access to the mmap-ed memory will cause a page fault and kill mmaptest.
+ Add code to cause a page-fault in a mmap-ed region to allocate a page of physical memory, read 4096 bytes of the relevant file into that page, and map it into the user address space. Read the file with readi, which takes an offset argument at which to read in the file (but you will have to lock/unlock the inode passed to readi). Don't forget to set the permissions correctly on the page. Run mmaptest; it should get to the first munmap.
+ Implement munmap: find the VMA for the address range and unmap the specified pages (hint: use uvmunmap). If munmap removes all pages of a previous mmap, it should decrement the reference count of the corresponding struct file. If an unmapped page has been modified and the file is mapped MAP_SHARED, write the page back to the file. Look at filewrite for inspiration.
+ Ideally your implementation would only write back MAP_SHARED pages that the program actually modified. The dirty bit (D) in the RISC-V PTE indicates whether a page has been written. However, mmaptest does not check that non-dirty pages are not written back; thus you can get away with writing pages back without looking at D bits.
+ Modify exit to unmap the process's mapped regions as if munmap had been called. Run mmaptest; mmap_test should pass, but probably not fork_test.
+ Modify fork to ensure that the child has the same mapped regions as the parent. Don't forget to increment the reference count for a VMA's struct file. In the page fault handler of the child, it is OK to allocate a new physical page instead of sharing a page with the parent. The latter would be cooler, but it would require more implementation work. Run mmaptest; it should pass both mmap_test and fork_test.
Run usertests to make sure everything still works.


### **1.2 实验要求（英文）：**

mmap和munmap系统调用允许UNIX程序对它们的地址空间进行详细的控制。它们可以用于在进程之间共享内存，将文件映射到进程地址空间中，并作为用户级页面故障方案的一部分，例如在讲座中讨论的垃圾收集算法中。在本实验中，您将向xv6添加mmap和munmap，重点是内存映射文件。

The manual page (run man 2 mmap) shows this declaration for mmap:

```
void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
```

mmap可以以多种方式调用，但本实验仅需要与内存映射文件相关的功能子集。您可以假设addr始终为零，这意味着内核应该决定将文件映射到的虚拟地址。mmap返回该地址，如果失败则返回0xffffffffffffffff。length是要映射的字节数；它可能与文件的长度不同。prot指示内存是否应映射为可读、可写和/或可执行；您可以假设prot为PROT_READ或PROT_WRITE或两者都有。flags将是MAP_SHARED或MAP_PRIVATE，前者表示对映射内存的修改应写回文件，后者表示不应写回。您不必实现flags中的任何其他位。fd是要映射的文件的打开文件描述符。您可以假设offset为零（它是要映射的文件中的起始点）。

如果映射同一个MAP_SHARED文件的进程不共享物理页面，也是可以的。

munmap(addr，length)应该删除指定地址范围内的mmap映射。如果进程修改了内存并将其映射为MAP_SHARED，则修改应首先写入文件。munmap调用可能仅涵盖mmap区域的一部分，但您可以假设它将在开头、结尾或整个区域取消映射（但不会在区域中间打洞）。

您应该实现足够的mmap和munmap功能，使mmaptest测试程序正常工作。如果mmaptest没有使用mmap功能，则无需实现该功能。

完成后，您应该看到以下输出：

```sh
$ mmaptest
mmap_test starting
test mmap f
test mmap f: OK
test mmap private
test mmap private: OK
test mmap read-only
test mmap read-only: OK
test mmap read/write
test mmap read/write: OK
test mmap dirty
test mmap dirty: OK
test not-mapped unmap
test not-mapped unmap: OK
test mmap two files
test mmap two files: OK
mmap_test: ALL OK
fork_test starting
fork_test OK
mmaptest: all tests succeeded
$ usertests
usertests starting
...
ALL TESTS PASSED
$ 
```

一些提示：

+ 首先，将_mmaptest添加到UPROGS中，并添加mmap和munmap系统调用，以使user/mmaptest.c编译通过。现在，只需从mmap和munmap返回错误即可。我们已经在kernel/fcntl.h中为您定义了PROT_READ等。运行mmaptest，它将在第一个mmap调用失败。
+ 惰性地填充页表，以响应页面故障。也就是说，mmap不应分配物理内存或读取文件。相反，在usertrap中（或由其调用的代码）的页面故障处理代码中执行此操作，就像惰性页面分配实验中一样。懒惰的原因是确保大文件的mmap速度快，并且可以映射大于物理内存的文件。
+ 跟踪每个进程的mmap映射。定义一个与Lecture 15中描述的VMA（虚拟内存区域）相对应的结构，记录由mmap创建的虚拟内存范围的地址、长度、权限、文件等。由于xv6内核在内核中没有内存分配器，因此可以声明一个固定大小的VMA数组，并根据需要从该数组中分配。大小为16应该足够。
+ 实现mmap：在进程的地址空间中找到一个未使用的区域，将文件映射到其中，并将VMA添加到进程的映射区域表中。VMA应包含指向正在映射的文件的struct file的指针；mmap应增加文件的引用计数，以便在关闭文件时不会使结构体消失（提示：请参阅filedup）。运行mmaptest：第一个mmap应该成功，但对mmap-ed内存的第一次访问将导致页面故障并杀死mmaptest。
+ 添加代码以使mmap-ed区域中的页面故障分配一个物理内存页面，将4096个字节的相关文件读入该页面，并将其映射到用户地址空间。使用readi读取文件，它需要一个偏移量参数来读取文件（但您将不得不锁定/解锁传递给readi的inode）。不要忘记在页面上正确设置权限。运行mmaptest；它应该到达第一个munmap。
+ 实现munmap：找到地址范围的VMA并取消映射指定的页面（提示：使用uvmunmap）。如果munmap删除了先前mmap的所有页面，则应减少相应的struct file的引用计数。如果已取消映射的页面已被修改并且文件被映射为MAP_SHARED，则将页面写回文件。查看filewrite以获得灵感。
+ 理想情况下，您的实现只会写回程序实际修改的MAP_SHARED页面。 RISC-V PTE中的dirty位（D）指示页面是否已写入。但是，mmaptest不会检查非脏页面是否被写回；因此，您可以在不查看D位的情况下写回页面。
+ 修改exit以取消映射进程的映射区域，就像调用munmap一样。运行mmaptest；mmap_test应该通过，但可能不会通过fork_test。
+ 修改fork以确保子进程具有与父进程相同的映射区域。不要忘记增加VMA的struct file的引用计数。在子进程的页面故障处理程序中，可以分配新的物理页面而不是与父进程共享页面。后者更酷，但需要更多的实现工作。运行mmaptest；它应该通过mmap_test和fork_test。
运行usertests以确保一切仍然正常工作。

### **1.2 实验思路与代码：**

首先大致梳理出mmap和unmap的实现机制

+ mmap这个系统调用本身是根据要求创建一段vma，但此时并没有将file中的内容拷贝到内存中并建立映射关系，具体建立映射关系的工作是在pagefault中完成的，当发生pagefault时，会根据vma的信息，将file中的对应页的内容拷贝到内存中，并建立映射关系。

+ 而unmap则是取消这个映射关系，并且释放内存。除此之外还要处理fork和exit的情况，fork时需要将父进程的vma拷贝到子进程中，exit时需要释放进程的vma。

以上是mmap的基本机制。具体实现上还有一些技术问题
```c
// mmap
void* mmap(void *addr, int length, int prot, int flags, int fdnum, int offset);

// munmap
int munmap(void *addr, int length);
```

首先解析mmap的参数，传入的addr是希望映射的内存地址（用户空间的va）的头，如果addr为0的话（这种情况下是大多数），那么由内存自行分配地址。要解析的第一个问题就是如何自行分配地址。

看一下用户空间va的分布图

```
    MAXVA -->   +----------------+  
                |    trapoline   |  PGSIZE
TRAPOLINE -->   +----------------+
                |    trapframe   |  PGSIZE
TRAPFRAME -->   +----------------+
                |                |
                |                |
                |                |
                |      heap      |
                |                |
                |                |
                |                |
                +----------------+
                |     stack      |  PGSIZE
                +----------------+
                |   guard page   |  PGSIZE
                +----------------+
                |                |
                |      data      |
                |                |
                +----------------+
                |                |
                |      text      |
                |                |
        0 -->   +----------------+   

```

我们在struct proc中设置一个max_addr,初始时设置为TRAPFRAME,每次取vma时都将其向下移动length的大小,并向下按照PGSIZE取整，获得起始的addr（这其实就模拟了heap的分配）

关于vma的设计，我们可以直接看mmap的参数，基本就有大概了
`proc.h`
```c
struct vma{
  struct spinlock lock;

  int isValid;
  uint64 addr;
  uint length;
  int prot;
  int flag;
  int off;
  struct file* f;
};
```

我们要在allocproc中将max_addr设置为TRAPFRAME，并且在中procinit给vma的锁做初始化

`proc.c`
```c
void
procinit(void)
{
  struct proc *p;
  int i;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->kstack = KSTACK((int) (p - proc));
      for(i=0;i<MAXVMA;i++)
        initlock(&(p->vma_list[i].lock),"vma_lock");
  }
}

static struct proc*
allocproc(void)
{
  struct proc *p;
  ...
found:
  ...
  p->max_addr=TRAPFRAME;
  ...
  return p;
}
```

在系统调用中实现基本的mmap函数，这一步非常的简单(如何实现一个系统调用的基本方法此处省略)
基本过程就是从proc的vma_list中找一个没用过的，分配一个vma即可
`kernel/sysfile.c`
```c
uint64
sys_mmap(void){
  uint64 addr;
  int len;
  int prot;
  int flags;
  int fd;
  int off;
  struct file* f;

  if(argaddr(0,&addr)<0 || argint(1,&len) || argint(2,&prot)<0
  || argint(3,&flags) || argfd(4,&fd,&f) || argint(0,&off))
  {
    return -1;
  }

  if(prot ==PROT_NONE)
    return -1;
  if( (prot & PROT_READ) && f->readable==0)
    return -1;
  if( (prot & PROT_WRITE)&& flags==MAP_SHARED && f->writable==0)
    return -1;
  //此处要做内存映射，将argaddr所指向的内存映射到fd的内存时
  //又不做映射了，采用vma处理的方式

  //当addr是0的时候，采用从trapframe向下生长的方式去解决
  struct proc* p=myproc();
  struct vma* vma;
  
  vma=alloc_vma(p);
  if(vma==0){
    printf("allocvma failed");
    return -1;
  }

  acquire(&vma->lock);
  vma->addr=PGROUNDDOWN(p->max_addr-len);//此处要向下取整
  vma->length=len;
  vma->flag=flags;//flag表示是否是share，如果是share，修改了，最后页释放的时候要执行写回操作
  vma->prot=prot;//prot表示页的可读性和可写性
  vma->off=off;
  vma->f=f;

  filedup(f);
  p->max_addr=vma->addr;

  release(&vma->lock);
  return vma->addr;
}
```
分配vma的方式如下（其实就是遍历）
`proc.c`
```c
struct vma* alloc_vma(struct proc *p){
  struct vma* vma;
  for(vma=p->vma_list;vma<p->vma_list+MAXVMA;vma++){
    acquire(&vma->lock);
    if(vma->isValid==0){
      release(&vma->lock);
      vma->isValid=1;
      return vma;
    }
    release(&vma->lock);  
  }
  return 0;
}
```

mmap实现的重点在于利用trap机制实现的类似cow的效果，当访问的地址不存在，会触发page fault机制。我们需要判断，当va在vma中合法的存着的时候，我们要申请一块内存，然后将对应的页块从file中拷贝到内存中，然后将va和pa的映射关系建立起来，这样就可以实现内存映射了。

```c
void
usertrap(void)
{
  ...
  if(r_scause()==8){
    ...
  }
  ...
  else if (r_scause()==13 || r_scause()==15)
  {
    uint64 va=r_stval();//访问错误的虚拟地址
    int i;
    if(va<p->sz){
      if((i=lz_alloc_handler(va,p->pagetable))<0)//此处是lazyalloc的代码，与mmap无关
        p->killed=1;
    }
    else{
      //判断是否为vma内的地址没有分配内存页所引发的trap
      struct vma* vma;
      int ishandle=-1;
      for(i=0;i<MAXVMA;i++){
        // acquire(&(vma->lock));
        vma=&(p->vma_list[i]);
        if(vma->isValid && vma->addr<=va && vma->addr+vma->length > va){
          //是vma所引发的缺页
          ishandle = vmatrap_handler(va,vma);
          break;
        }
        // release(&(vma->lock));
      }
      if(ishandle==-1){
        printf("usertrap(): scause %p pid=%d\n", r_scause(), p->pid);
        printf("handle va:%p failed\n",va);
        printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
        p->killed=1;
      } 
    }
  }
  ...
}
```
`file.c`
```c
int vmatrap_handler(uint64 va,struct vma* vma)
{
  //针对该块申请空间，并且将文件中的内容拷贝过来
  
  uint64 pa;
  uint64 cpva;
  uint64 vsz;
  struct file* f=vma->f;
  struct proc* p;
  int i;
  int r;
  int off;
  int perm;

  va=PGROUNDDOWN(va);
  vsz=vma->addr+vma->length - va;
  if(vsz>PGSIZE)
    vsz=PGSIZE;
  
  //申请物理空间进行映射
  //acquire()
  if((pa=(uint64)kalloc())==0){
    printf("vmatrap_handler error: physical addr alloc failed");
    return -1;
  }
  memset((void*)pa,0,PGSIZE);
  p=myproc();

  //该页的权限填写
  perm=PTE_V | PTE_U;
  if(vma->prot & PROT_READ)
    perm |=PTE_R;
  if(vma->prot & PROT_WRITE)
    perm |=PTE_W;
  if(vma->prot & PROT_EXEC)
    perm |=PTE_X;

  //映射
  if((i=mappages(p->pagetable,va,vsz,pa,perm))<0) {
    printf("vmatrap_handler error: mappages failed");
    return -1;
  }

  //由于munmap的缘故导致vma的开头不再是PGSIZE取整的情况，所以写入时要做一下调整
  if(va<vma->addr){
    vsz=vsz-(vma->addr-va);
    cpva=vma->addr;
    off=vma->off;
  }
  else{
    cpva=va;
    off=vma->off+(va-vma->addr);
  }

  //映射完之后，将file的文件内容拷贝到相应的物理空间(目前只支持文件时inode的情况)
  if(f->type==FD_INODE){
    ilock(f->ip);
    if((r=readi(f->ip,1,cpva,off,vsz))<0){
      printf("vmatrap_handler error: read something from file failed");
      iunlock(f->ip);
      return -1;
    }
    iunlock(f->ip);
  }
  else{
    printf("file is not inode");
    return -1;
  }
  return 0;
}
```

然后是munmap的实现，实现munmap要注意写回机制，如果是share的情况，要将修改的内容写回到文件中
过程也很简单，不过涉及到如果只是munmap掉部分一个vma中的一段地址的情况，要将剩下的连续地址各自攒成自己的vma
```c
uint64
sys_munmap(void){
  uint64 addr;
  int len;
  struct proc* p;
  int i;
  int npage;
  struct vma* vma;
  struct vma* n_vma;
  struct file* f;
  int off;

  if(argaddr(0,&addr)<0 || argint(1,&len)<0)
    return -1;
  
  if(addr%PGSIZE!=0){
    printf("addr need align at PGSIZE");
    return -1;
  }
  
  p=myproc();
  //默认munmap不会跨越两个vma区间？
  vma=0;
  for(i=0;i<MAXVMA;i++){
    vma=&p->vma_list[i];
    if(vma->isValid && addr>=vma->addr && addr<vma->addr+vma->length)
    {
      if(addr+len > vma->addr+vma->length)
      {
        printf("can't modify across moere than 1 vma");
        return -1;
      }
      break;
    }
  }
  if(vma==0){
    printf("no area is mmaped");
    return 0;
  }

  //如果unmap的是中间，那么两头要新建自己的vma
  if(vma->addr < addr){
    n_vma=alloc_vma(p);
    if(n_vma==0)
      return -1;
    *n_vma=*vma;
    n_vma->length=addr-vma->addr;
    filedup(n_vma->f);
  }

  if(vma->addr + vma->length > addr+len){
    n_vma=alloc_vma(p);
    if(n_vma==0)
      return -1;
    *n_vma=*vma;//先将基本的属性搞过去
    n_vma->addr=addr+len;
    n_vma->length=vma->addr + vma->length - (addr+len);
    n_vma->off=vma->off+addr+len-vma->addr;
    filedup(n_vma->f);
  }

  f=vma->f;
  off=vma->off+addr-vma->addr;
  if(vma->flag==MAP_SHARED)
    writeBk(addr,len,f,off);
  
  fileclose(vma->f);
  //将页取消映射
  npage=(PGROUNDUP(addr+len)-addr)/PGSIZE;
  uvmunmap(p->pagetable,addr,npage,1);

  //回收vma
  acquire(&vma->lock);
  vma->isValid=0;
  release(&vma->lock);

  //虚拟内存不回收哈
  return 0;
}
```

file.c
```c
int writeBk(uint64 addr,int len,struct file* f,int off){
  //需要将对应的页写回
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    int r;
    while(i < len){
      int n1 = len - i;
      if(n1 > max)
        n1 = max;
      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, off, n1)) > 0)
        off += r;
      iunlock(f->ip);
      end_op();
      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }

    return 0;
}
```

对于fork和exit我们也要做出相应的调整
```c
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  struct vma *vma;
  

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  np->max_addr=p->max_addr;
  //对于vma也要拷贝过来
  for(i=0;i<MAXVMA;i++){
    vma=&(p->vma_list[i]);
    if(vma->isValid){
      memmove(&np->vma_list[i],vma,sizeof(struct vma));
      filedup(vma->f);
    }
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

//清除所有的vma的map关系，对于共享的vma，需要将对应的页写回
void
exit(int status)
{
  struct proc *p = myproc();
  struct vma *vma;

  for (int i = 0; i < MAXVMA;i++){
    vma = &(p->vma_list[i]);
    if (vma->isValid){
      uint64 pgsz = (PGROUNDUP(vma->addr + vma->length) - vma->addr) / PGSIZE;
      if(vma->flag==MAP_SHARED){
        writeBk(vma->addr,vma->length,vma->f,vma->off);
      }
      uvmunmap(p->pagetable, vma->addr, pgsz, 1);
      fileclose(vma->f);
      vma->isValid = 0;
    }
  }
    if (p == initproc)
      panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}
```
vm.c

对于uvmunmap,对于非法页直接continue就好
```c
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    ...
    if((*pte & PTE_V) == 0){
      //printf("not mapped page\n");
      continue;
    }
    ...
  }
}

```

