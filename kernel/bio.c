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

#define NBUCKETS 13   // 素数个哈希桶

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf hashbucket[NBUCKETS];
} bcache;


int hash(int blockno)
{
  return blockno % NBUCKETS;
}

void
binit(void)
{
  struct buf *b;
  int i;
  int group;

  for (i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
  }
  

  // Create linked list of buffers
  for (i = 0; i < NBUCKETS; i++)
  {
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    group= hash(b->blockno);
    b->next = bcache.hashbucket[group].next;
    b->prev = &bcache.hashbucket[group];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[group].next->prev = b;
    bcache.hashbucket[group].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int group = hash(blockno);
  int i;

  acquire(&bcache.lock[group]);

  // Is the block already cached?
  for(b = bcache.hashbucket[group].next; b != &bcache.hashbucket[group]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[group]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  // 当bget()查找数据块未命中时，bget可从其他哈希桶选择一个未被使用的缓存块，移入到当前的哈希桶链表中使用
  for (i = 0; i < NBUCKETS; i++)
  {
    if (i == group)
    {
      continue;
    }

    acquire(&bcache.lock[i]);

    for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.lock[i]);

        b->next = bcache.hashbucket[group].next;
        b->prev = &bcache.hashbucket[group];
        bcache.hashbucket[group].next->prev = b;
        bcache.hashbucket[group].next = b;

        release(&bcache.lock[group]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
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
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  int group = hash(b->blockno);

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock[group]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[group].next;
    b->prev = &bcache.hashbucket[group];
    bcache.hashbucket[group].next->prev = b;
    bcache.hashbucket[group].next = b;
  }
  release(&bcache.lock[group]);
}

void
bpin(struct buf *b) {
  int group = hash(b->blockno);
  acquire(&bcache.lock[group]);
  b->refcnt++;
  release(&bcache.lock[group]);
}

void
bunpin(struct buf *b) {
  int group = hash(b->blockno);
  acquire(&bcache.lock[group]);
  b->refcnt--;
  release(&bcache.lock[group]);
}


