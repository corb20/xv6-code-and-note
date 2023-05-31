// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

uint64 CMemSize;

struct run {
  struct run *next;
};

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

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}


// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
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

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
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
