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

struct run {
  struct run *next;
};

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

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
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

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  uint64 pa;

  acquire(&kmem.lock);
  r = kmem.freelist;
  pa = (uint64)r;

  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  acquire(&page_ref[refIndex(pa)].lock);
  page_ref[refIndex(pa)].reftimes = 1;
  release(&page_ref[refIndex(pa)].lock);
  if (r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


void
krefalloc(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kRef Alloc");

  acquire(&(page_ref[refIndex((uint64)pa)].lock));
  page_ref[refIndex((uint64)pa)].reftimes++;
  release(&(page_ref[refIndex((uint64)pa)].lock));
}

int
get_kreftimes(void* pa){
  int ans;
  acquire(&(page_ref[refIndex((uint64)pa)].lock));
  ans = page_ref[refIndex((uint64)pa)].reftimes;
  release(&(page_ref[refIndex((uint64)pa)].lock));
  return ans;
}