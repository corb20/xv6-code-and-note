// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct {
  struct spinlock lock;
  struct buf head;
}bufbuck[BUCKSIZE];

int hash(int x){
  return x%BUCKSIZE;
}

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

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
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
  

  //从别的桶里面去抢
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
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

void
bpin(struct buf *b) {
  int bki=hash(b->blockno);
  acquire(&bufbuck[bki].lock);
  b->refcnt++;
  release(&bufbuck[bki].lock);
}

void
bunpin(struct buf *b) {
  int bki=hash(b->blockno);
  acquire(&bufbuck[bki].lock);
  b->refcnt--;
  release(&bufbuck[bki].lock);
}


