#include "unity.h"
#include "kmalloc.h"
#include "kmalloc_util.h"
#include <string.h>

#define HEAPSIZE 256

uint8_t heap[HEAPSIZE];



void debugDump(char *desc, void *addr, int len) 
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("  %s\n", buff);

            // Output the offset.
            printf("  %08x ", (int) &pc[i]);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buff[i % 16] = '.';
        } else {
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  %s\n", buff);
}


void setUp(void)
{
   memset(heap, 0, HEAPSIZE);
   kmalloc_init((void *) heap, HEAPSIZE);
}

void tearDown(void)
{
}

void test_InitialHeap(void)
{
  kmchunk_ptr chunk = (kmchunk_ptr) heap;
  kmchunk_ptr bin[SBIN_COUNT] = {NULL};
  struct kmalloc_state state =  kmalloc_debug_getstate();


 
  TEST_ASSERT_BITS (FLAGMASK, PINUSE , chunk->header);
  TEST_ASSERT_EQUAL(HEAPSIZE - 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR (chunk , chunk->next);
  TEST_ASSERT_EQUAL_PTR (chunk , chunk->prev);

  bin[calc_bin_no(HEAPSIZE - 8)] = chunk;
  TEST_ASSERT_EQUAL_PTR_ARRAY(bin, state.bin, SBIN_COUNT);

  kmalloc_chunk_iterate(&chunk);

  TEST_ASSERT_NOT_EQUAL(NULL, chunk);
  TEST_ASSERT_EQUAL(0, chunk->header);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_NULL (chunk);
  
}

void test_SimpleMallocFree(void)
{
  kmchunk_ptr chunk = (kmchunk_ptr) heap;
  kmchunk_ptr bin[SBIN_COUNT] = {NULL};
  struct kmalloc_state state;



  uint8_t *mem;
  mem = (uint8_t*) kmalloc(32 + 4);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE | CINUSE , chunk->header);
  TEST_ASSERT_EQUAL(32 + 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR(mem, &(chunk->next));
  
  kmalloc_chunk_iterate(&chunk);
  mem[32] = 0xDD;
  mem[33] = 0xCC;
  mem[34] = 0xBB;
  mem[35] = 0xAA;
  TEST_ASSERT_EQUAL_HEX(0xAABBCCDD, chunk->prev_foot);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE , chunk->header);
  TEST_ASSERT_EQUAL(HEAPSIZE - 8 - (32 + 8), GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR (chunk , chunk->next);
  TEST_ASSERT_EQUAL_PTR (chunk , chunk->prev);
  
  state =  kmalloc_debug_getstate();
  bin[calc_bin_no(HEAPSIZE - 8 - (32 + 8))] = chunk;
  TEST_ASSERT_EQUAL_PTR_ARRAY(bin, state.bin, SBIN_COUNT);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_NOT_EQUAL(NULL, chunk);
  TEST_ASSERT_EQUAL(0, chunk->header);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_NULL (chunk); 
  
  kfree(mem);

  //state should be the same as initial state
  test_InitialHeap();

}

void test_MallocTooBig(void)
{
  //allocate too large block of mem
  uint8_t *mem;
  mem = (uint8_t*) kmalloc(HEAPSIZE);
  TEST_ASSERT_EQUAL_PTR(NULL, mem);

  //heap should not have been changed
  test_InitialHeap();

}

void test_MallocMaxSize(void)
{

  kmchunk_ptr chunk = (kmchunk_ptr) heap;
  kmchunk_ptr bin[SBIN_COUNT] = {NULL};
  struct kmalloc_state state;

  uint8_t *mem;
  mem = (uint8_t*) kmalloc(HEAPSIZE - 12);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE | CINUSE , chunk->header);
  TEST_ASSERT_EQUAL(HEAPSIZE - 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR(mem, &(chunk->next));
  
  kmalloc_chunk_iterate(&chunk);

  TEST_ASSERT_NOT_EQUAL(NULL, chunk);
  TEST_ASSERT_EQUAL(0, chunk->header);

  mem[HEAPSIZE - 16] = 0xDD;
  mem[HEAPSIZE - 15] = 0xCC;
  mem[HEAPSIZE - 14] = 0xBB;
  mem[HEAPSIZE - 13] = 0xAA;
  TEST_ASSERT_EQUAL_HEX(0xAABBCCDD, chunk->prev_foot);

  
  state =  kmalloc_debug_getstate();
  TEST_ASSERT_EQUAL_PTR_ARRAY(bin, state.bin, SBIN_COUNT);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_NULL (chunk); 

  kfree(mem);

  //state should be the same as initial state
  test_InitialHeap();

}

void test_MultipleMalloc(void) {
  void *mem1, *mem2, *mem3, *mem4;
  kmchunk_ptr chunk = (kmchunk_ptr) heap;
  kmchunk_ptr bin[SBIN_COUNT] = {NULL};
  struct kmalloc_state state;

  mem1 = kmalloc(16);
  mem2 = kmalloc(32);
  mem3 = kmalloc(64);
  mem4 = kmalloc(8);



  TEST_ASSERT_BITS (FLAGMASK, PINUSE | CINUSE , chunk->header);
  TEST_ASSERT_EQUAL(16 + 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR(mem1  , &(chunk->next));
  kmalloc_chunk_iterate(&chunk);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE | CINUSE , chunk->header);
  TEST_ASSERT_EQUAL(32 + 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR(mem2  , &(chunk->next));
  kmalloc_chunk_iterate(&chunk);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE | CINUSE , chunk->header);
  TEST_ASSERT_EQUAL(64 + 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR(mem3  , &(chunk->next));
  kmalloc_chunk_iterate(&chunk);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE | CINUSE , chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR(mem4  , &(chunk->next));
  kmalloc_chunk_iterate(&chunk);

  TEST_ASSERT_BITS (FLAGMASK, PINUSE , chunk->header);
  TEST_ASSERT_EQUAL(HEAPSIZE - 160, GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_EQUAL_PTR (chunk , chunk->next);
  TEST_ASSERT_EQUAL_PTR (chunk , chunk->prev);

  state =  kmalloc_debug_getstate();
  bin[calc_bin_no(HEAPSIZE - 160)] = chunk;
  TEST_ASSERT_EQUAL_PTR_ARRAY(bin, state.bin, SBIN_COUNT);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_EQUAL(0, chunk->header);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_NULL (chunk); 

  kfree(mem1);
  kfree(mem2);
  kfree(mem3);
  kfree(mem4);

  //state should be the same as initial state
  test_InitialHeap();
}


void test_MultipleFreeCoalesceTop(void) {
  void *mem1, *mem2, *mem3;
  kmchunk_ptr chunk, chunk1,  chunk3;
  struct kmalloc_state state;

  mem1 = kmalloc(8);
  mem2 = kmalloc(8);
  mem3 = kmalloc(8);


  

  kfree(mem1);

  chunk1  = chunk = (kmchunk_ptr) heap;
  TEST_ASSERT_BITS (FLAGMASK, PINUSE, chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, GETCHUNKSIZE(chunk->header));
  state =  kmalloc_debug_getstate();
  TEST_ASSERT_EQUAL_PTR(chunk1, state.bin[0]);
  TEST_ASSERT_EQUAL_PTR(chunk1, chunk1->next);
  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_BITS (FLAGMASK, CINUSE, chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, chunk->prev_foot);
  kmalloc_chunk_iterate(&chunk);
  chunk3 = chunk;  
   
  kfree(mem2); //chunk2  should coalesce with chunk1
 
  TEST_ASSERT_BITS (FLAGMASK, PINUSE, chunk1->header);
  TEST_ASSERT_EQUAL(2 * (8 + 8), GETCHUNKSIZE(chunk1->header));
  state =  kmalloc_debug_getstate();
  TEST_ASSERT_EQUAL_PTR(NULL, state.bin[0]);
  TEST_ASSERT_EQUAL_PTR(chunk1, state.bin[2]);
  TEST_ASSERT_EQUAL_PTR(chunk1,chunk1->next);

  chunk = chunk1;
  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_EQUAL_PTR(chunk, chunk3);
  TEST_ASSERT_BITS (FLAGMASK, CINUSE, chunk->header);
  TEST_ASSERT_EQUAL(2 * (8 + 8), chunk->prev_foot);


  kfree(mem3);  
  
  //state should be the same as initial state
  test_InitialHeap();
}

void test_MultipleFreeCoalesceBottom(void) {
  void *mem1, *mem2, *mem3;
  kmchunk_ptr chunk, chunk1, chunk2, chunk3;
  struct kmalloc_state state;

  mem1 = kmalloc(8);
  mem2 = kmalloc(8);
  mem3 = kmalloc(8);

  TEST_FAIL();
  

  kfree(mem2);

 
   
  kfree(mem1); //chunk2  should coalesce with chunk1
 
 

  kfree(mem3);  
  
  //state should be the same as initial state
  test_InitialHeap();
}

void test_MultipleFreeCoalesceBoth(void) {
  void *mem1, *mem2, *mem3, *mem4;
  kmchunk_ptr chunk, chunk1, chunk3, chunk4;
  struct kmalloc_state state;

  mem1 = kmalloc(8);
  mem2 = kmalloc(8);
  mem3 = kmalloc(8);
  mem4 = kmalloc(8);

  

  kfree(mem1);

  chunk  = chunk1 = (kmchunk_ptr) heap;
  TEST_ASSERT_BITS (FLAGMASK, PINUSE, chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, GETCHUNKSIZE(chunk->header));
  state =  kmalloc_debug_getstate();
  TEST_ASSERT_EQUAL_PTR(chunk1, state.bin[0]);
  kmalloc_chunk_iterate(&chunk);
  
  TEST_ASSERT_BITS (FLAGMASK, CINUSE, chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, chunk->prev_foot);
  kmalloc_chunk_iterate(&chunk);
  
  chunk3 = chunk;
  
  kfree(mem3);
 
  TEST_ASSERT_BITS (FLAGMASK, PINUSE, chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, GETCHUNKSIZE(chunk->header));
  state =  kmalloc_debug_getstate();
  TEST_ASSERT_EQUAL_PTR(chunk3, state.bin[0]);
  TEST_ASSERT_EQUAL_PTR(chunk1,chunk3->next);
  TEST_ASSERT_EQUAL_PTR(chunk3,chunk1->next);

  kmalloc_chunk_iterate(&chunk);
  chunk4 = chunk;

  TEST_ASSERT_BITS (FLAGMASK, CINUSE, chunk->header);
  TEST_ASSERT_EQUAL(8 + 8, chunk->prev_foot);
  kmalloc_chunk_iterate(&chunk);


  kfree(mem2);  //chunk1 2 and 3 should coalesce together
  
  chunk = chunk1;
  TEST_ASSERT_EQUAL(3* (8 + 8), GETCHUNKSIZE(chunk->header));
  TEST_ASSERT_BITS (FLAGMASK, PINUSE, chunk->header);
  state =  kmalloc_debug_getstate();
  TEST_ASSERT_EQUAL_PTR(NULL, state.bin[0]);
  TEST_ASSERT_EQUAL_PTR(chunk1,state.bin[4]);
  TEST_ASSERT_EQUAL_PTR(chunk1,chunk1->next);

  kmalloc_chunk_iterate(&chunk);
  TEST_ASSERT_EQUAL_PTR(chunk4,chunk);
  TEST_ASSERT_BITS (FLAGMASK, CINUSE, chunk->header);
  TEST_ASSERT_EQUAL(3* (8 + 8), chunk->prev_foot);

  kfree(mem4);

  //state should be the same as initial state
  test_InitialHeap();
}

void test_MallocSplit(void) {
    TEST_FAIL();
}

void test_MallocShouldNotSplitSmallChunk(void) {
    TEST_FAIL();
}

void test_MallocLastChunk(void) {
    TEST_FAIL();
}