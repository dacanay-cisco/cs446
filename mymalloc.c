//Cisco Dacanay
//PA4 - My Malloc
//11/14/23

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <ctype.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#define MBLOCK_HEADER_SZ offsetof(mblock_t, payload)

typedef struct _mblock_t {
    struct _mblock_t * prev;
    struct _mblock_t * next;
    size_t size;
    int status;
    void * payload;
} mblock_t;

typedef struct _mlist_t {
    mblock_t * head;
} mlist_t;

void printMemList(const mblock_t* head);
void * mymalloc(size_t size);
void myfree(void * ptr);
mblock_t * findLastMemlistBlock();
mblock_t * findFreeBlockOfSize(size_t size);
void splitBlockAtSize(mblock_t * block, size_t newSize);
void coallesceBlockPrev(mblock_t * freeBlock);
void coallesceBlockNext(mblock_t * freeBlock);
mblock_t * growHeapBySize(size_t size);

mlist_t mlist = {.head = NULL};

int main(int argc, char* argv[]) {
    void * p1 = mymalloc(10);
    void * p2 = mymalloc(100);
    void * p3 = mymalloc(200);
    void * p4 = mymalloc(500);
    myfree(p3); p3 = NULL;
    myfree(p2); p2 = NULL;
    void * p5 = mymalloc(150);
    void * p6 = mymalloc(500);
    printMemList(mlist.head);
    myfree(p4); p4 = NULL;
    myfree(p5); p5 = NULL;
    myfree(p6); p6 = NULL;
    myfree(p1); p1 = NULL;

    printMemList(mlist.head);

    return 0;
}

void printMemList(const mblock_t* head) {
  const mblock_t* p = head;
  
  size_t i = 0;
  while(p != NULL) {
    printf("[%ld] p: %p\n", i, p);
    printf("[%ld] p->size: %ld\n", i, p->size);
    printf("[%ld] p->status: %s\n", i, p->status > 0 ? "allocated" : "free");
    printf("[%ld] p->prev: %p\n", i, p->prev);
    printf("[%ld] p->next: %p\n", i, p->next);
    printf("___________________________\n");
    ++i;
    p = p->next;
  }
  printf("===========================\n");
}

void * mymalloc(size_t size) {
    mblock_t * freeBlock = findFreeBlockOfSize(size);
    if(freeBlock == NULL) {
        freeBlock = growHeapBySize(size);
        if(freeBlock == (void *) -1) {
            printf("Error: Could not grow heap.\n");
            return (void *) -1;
        }
    }
    splitBlockAtSize(freeBlock, size);
    return &(freeBlock->payload);
}

void myfree(void * ptr) {
    mblock_t * allocBlock = (mblock_t *) (ptr - MBLOCK_HEADER_SZ);
    allocBlock->payload = NULL;
    allocBlock->status = 0;
    coallesceBlockNext(allocBlock);
    coallesceBlockPrev(allocBlock);
}

mblock_t * findLastMemlistBlock() {
    mblock_t * blockPtr = mlist.head;
    if(blockPtr == NULL) {
        return NULL;
    }
    while(blockPtr->next != NULL) {
        blockPtr = blockPtr->next;
    }
    return blockPtr;
}

mblock_t * findFreeBlockOfSize(size_t size) {
    mblock_t * blockPtr = mlist.head;
    if(blockPtr == NULL) {
        return NULL;
    }
    while(blockPtr->status == 1 || blockPtr->size < size) {
        if(blockPtr->next == NULL) {
            return NULL;
        }
        else {
            blockPtr = blockPtr->next;
        }
    }
    return blockPtr;
}

void splitBlockAtSize(mblock_t * block, size_t newSize) {
    if(newSize + sizeof(mblock_t) + 1 <= block->size) {     //dont create smaller block if remaining size will be smaller than header + 1 byte
        mblock_t * remaining_block = (mblock_t *) ((void *)block + newSize + MBLOCK_HEADER_SZ);
        remaining_block->prev = block;
        remaining_block->next = block->next;
        remaining_block->status = 0;
        remaining_block->size = block->size - newSize - MBLOCK_HEADER_SZ;

        if(block->next != NULL) {
            block->next->prev = remaining_block;
        }
        block->next = remaining_block;
        block->size = newSize;
    }
    block->status = 1;
}

void coallesceBlockPrev(mblock_t * freeBlock) {
    mblock_t * prevBlock = freeBlock->prev;
    if(prevBlock != NULL && prevBlock->status == 0) {   //short circuit to prevent NULL->status
        if(freeBlock->next != NULL) {
            freeBlock->next->prev = prevBlock;
        }
        prevBlock->next = freeBlock->next;
        prevBlock->size = prevBlock->size + freeBlock->size + MBLOCK_HEADER_SZ;
    }
}

void coallesceBlockNext(mblock_t * freeBlock) {
    mblock_t * nextBlock = freeBlock->next;
    if(nextBlock != NULL && nextBlock->status == 0) {
        if(nextBlock->next != NULL) {
            nextBlock->next->prev = freeBlock;
        }
        freeBlock->next = nextBlock->next;
        freeBlock->size = freeBlock->size + nextBlock->size + MBLOCK_HEADER_SZ;
    }
}

mblock_t * growHeapBySize(size_t size) {
    void * start_addr;
    size_t growSize;
    if(size + MBLOCK_HEADER_SZ < 1000) {    //grow heap by max of 1kb or size needed
        growSize = 1000;
    }
    else {
        growSize = size + MBLOCK_HEADER_SZ;
    }
    start_addr = sbrk(growSize);
    if(start_addr == (void *) -1) {
        return (void *) -1;
    }
    mblock_t * newBlock = (mblock_t *) start_addr;
    newBlock->prev = findLastMemlistBlock();
    newBlock->next = NULL;
    newBlock->status = 0;
    newBlock->size = growSize;

    if(mlist.head == NULL) {
        mlist.head = newBlock;
    }
    else {
        findLastMemlistBlock()->next = newBlock;
    }
    
    return newBlock;
}