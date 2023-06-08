//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"

#include "fcntl.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}

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