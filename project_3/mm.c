
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
	"20190388",
	"Sangook Baek",
	"sky9763@sogang.ac.kr",
};


#define ALIGNMENT 8


#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


#define WSIZE 4				
#define DSIZE 8				
#define CHUNKSIZE (1<<12)	

#define MAX(x, y) ((x) > (y) ? (x) : (y))


#define PACK(size, alloc) ((size) | (alloc))


#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))


#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)


#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)


#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

//next, prev의 주소를 bp에서 구해줌
#define NEXT_EX(bp) GET(HDRP(bp) + WSIZE)
#define PRE_EX(bp) GET(HDRP(bp) + DSIZE)

//각각 heap과 free list를 포인트하는 포인터
static char *heap_listp = 0;	
static char *free_listp = NULL;		

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
void remove_block(void *bp);
void insert_block(void *bp);

int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *) - 1)
        return -1;
    PUT(heap_listp, 0);					//alignment			
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE * 2, 1));//prologue header
    PUT(heap_listp + (2 * WSIZE), NULL); //next
    PUT(heap_listp + (3 * WSIZE), NULL);//prev
    PUT(heap_listp + (4 * WSIZE), PACK(DSIZE * 2, 1));	//prologue header
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));	//epilogue
    free_listp = heap_listp + DSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;


    return 0;
}


void *mm_malloc(size_t size)
{
    size_t asize;	
    size_t extendsize;	
    char *bp;

	if (heap_listp == 0) {
		mm_init();
	}

	if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE ) + (DSIZE-1)) / DSIZE);


    if ((bp = find_fit(asize)) != NULL) {
        bp = place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    bp = place(bp, asize);
;
    return bp;
}


void mm_free(void *bp)
{
	if (bp == 0)
		return;

    size_t size = GET_SIZE(HDRP(bp));

	if (heap_listp == 0) {
		mm_init();
	}

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}


void *mm_realloc(void *ptr, size_t size) {
  void* newptr;
  void *oldptr = ptr;
  void *prevptr;
  size_t oldsize = GET_SIZE(HDRP(ptr));
  size_t newsize = size + 2 * WSIZE;
  if(size == 0) { //free와 동일
		mm_free(ptr);
		return 0;
	}
  if (!ptr){ //할당
    return mm_malloc(size);
  }
  if (newsize<=oldsize){ //그냥 반환하자
    return ptr;
  }
  else{
    size_t csize;
    if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && ((csize = oldsize + GET_SIZE(HDRP(NEXT_BLKP(ptr))))) >= newsize){ 
      remove_block(NEXT_BLKP(ptr)); 
      PUT(HDRP(ptr), PACK(csize, 1)); 
      PUT(FTRP(ptr), PACK(csize, 1)); 
      return ptr; 
    }
    // else if (!GET_ALLOC(HDRP(PREV_BLKP(ptr))) && ((csize = oldsize + GET_SIZE(HDRP(PREV_BLKP(ptr))))) >= newsize){
    //   newptr = mm_malloc(oldsize);
    //   memcpy(newptr, oldptr, oldsize);
    //   prevptr = PREV_BLKP(ptr);
    //   remove_block(PREV_BLKP(ptr));
    //   free(oldptr);
    //   memcpy(prevptr, newptr, oldsize);
    //   free(newptr);
    //   PUT(HDRP(prevptr), PACK(csize, 0)); 
    //   PUT(FTRP(prevptr), PACK(csize, 0)); 
    //   return prevptr;
    // }
    else{ 
      newptr = mm_malloc(size);
      memcpy(newptr, oldptr, oldsize);
      mm_free(oldptr);
      return newptr;
    }

  }
}


static void *extend_heap(size_t words)
{
    char * bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}


static void *place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));
	size_t rsize = csize - asize;

    remove_block(bp);

	if (rsize <= 4 * WSIZE) {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    
	//기준?? 100으로 정해 기준에 따라 할당한 블럭을 뒤로 놓자
	else if (asize >= 100)
    {
        PUT(HDRP(bp), PACK(rsize, 0));
        PUT(FTRP(bp), PACK(rsize, 0));
        insert_block(bp);

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

    }

	else
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        PUT(HDRP(NEXT_BLKP(bp)), PACK(rsize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(rsize, 0));

        insert_block(NEXT_BLKP(bp));
    }
    return bp;
}


static void *find_fit(size_t asize) { //fisrt fit
    void *tmp_bp = free_listp;
    while (!GET_ALLOC(HDRP(tmp_bp)))
    {
        if (GET_SIZE(HDRP(tmp_bp)) >= asize)
            return tmp_bp;
        tmp_bp = NEXT_EX(tmp_bp);
    }
    return NULL;
}


static void *coalesce(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    if (GET_ALLOC(FTRP(PREV_BLKP(bp))) && !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
        remove_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!GET_ALLOC(FTRP(PREV_BLKP(bp))) && GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
        remove_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else if (!GET_ALLOC(FTRP(PREV_BLKP(bp))) && !GET_ALLOC(HDRP(NEXT_BLKP(bp))))  {									
        remove_block(PREV_BLKP(bp));
        remove_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_block(bp);
    return bp;
}


void insert_block(void *bp){ //블럭을 새로 삽입 LIFO 사용
    PRE_EX(bp) = NULL;
    NEXT_EX(bp) = free_listp;
    PRE_EX(free_listp) = bp;
    free_listp = bp;
}


void remove_block(void *bp){ //block을 free 시킴
	if (!PRE_EX(bp)) { //이때 첫 번째 block인가?
      free_listp = NEXT_EX(bp);
      PRE_EX(NEXT_EX(bp)) = NULL;   
    }
    else { //아닌가??
      NEXT_EX(PRE_EX(bp)) = NEXT_EX(bp);
      PRE_EX(NEXT_EX(bp)) = PRE_EX(bp);
    }
}