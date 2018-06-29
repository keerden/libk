#include "unity.h"
#include "kmalloc.h"
#include "kmalloc_util.h"
#include "testsuite.h"
#include <string.h>

#define HEAPSIZE 9216

uint8_t heap[HEAPSIZE] __attribute__((aligned(8)));

intptr_t increment;
int cb_ok, cb_fail, cb_abort;

void* callback_ok (intptr_t i){
    cb_ok++;
    increment = i;
    return (void*) NULL;
}

void* callback_fail (intptr_t i){
    cb_fail++;
    increment = i;
    return (void*) -1;
}

void callback_abort (void){
    cb_abort++;
}

void setUp(void)
{
    memset(heap, 0xAA, HEAPSIZE);
    increment = cb_ok = cb_fail = cb_abort = 0;
}

void tearDown(void)
{
}


void test_MallocMaxSize(void)   //allocate all space within the heap
{
    struct kmalloc_state state;
    void *mem;
    __KMALlOC_SET_SBRK_CALLBACK(callback_ok);
    kmalloc_init((void *)heap, 512);

    mem = kmalloc(512 + 4 - 8 - DUMMYSIZE);
    TEST_ASSERT_NOT_NULL(mem);
    TEST_ASSERT_EQUAL(0, cb_ok);

    memset(mem, '1', 512 + 4 - 8 - DUMMYSIZE);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(512, state.heap_size);
    

    test_chunkinfo expected[2] = {
        {USED, 512 - DUMMYSIZE, mem},
        {TC, 0, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));
    
    
    //now check if free correctly increases the empty top chunk

    kfree(mem);
    TEST_ASSERT_EQUAL(0, cb_ok);

    state = kmalloc_debug_getstate();

    test_chunkinfo expected2[1] = {
        {TC, 512 - DUMMYSIZE, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected2, 1, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));


}

void test_MallocOutOfMem(void)  //allocate more than heapsize and more than max mem
{
    struct kmalloc_state state;
    void *mem;
    __KMALlOC_SET_SBRK_CALLBACK(callback_fail);
    kmalloc_init((void *)heap, 512);

    mem = kmalloc(580);
    TEST_ASSERT_NULL(mem);
    TEST_ASSERT_EQUAL(1, cb_fail);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(512, state.heap_size);

    test_chunkinfo expected[1] = {
        {TC, 512 - DUMMYSIZE, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 1, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));
}

void test_MallocOverLimit(void)  //allocate more than heapsize, but less than max mem
{
    struct kmalloc_state state;
    void *mem;
    __KMALlOC_SET_SBRK_CALLBACK(callback_ok);
    kmalloc_init((void *)heap, 512);

    mem = kmalloc(520 + 4 - 8 - DUMMYSIZE);
    TEST_ASSERT_NOT_NULL(mem);
    TEST_ASSERT_EQUAL(1, cb_ok);

    memset(mem, '1', 520 + 4 - 8 - DUMMYSIZE);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(520, state.heap_size);
    test_chunkinfo expected[2] = {
        {USED, 520 - DUMMYSIZE, mem},
        {TC, 0, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 520));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, 520));
    TEST_ASSERT_FALSE(check_bins(heap,  520, state));
    
}


void test_FreeNULLptr(void)
{
  struct kmalloc_state state;
    void *mem;
    __KMALlOC_SET_ABORT_CALLBACK(callback_abort);
    kmalloc_init((void *)heap, 512);
    mem = kmalloc(32);

    kfree(NULL);

    TEST_ASSERT_EQUAL(0, cb_abort);

    state = kmalloc_debug_getstate();

    test_chunkinfo expected[2] = {
        {USED, 32 + 8, mem},
        {TC, 512 - DUMMYSIZE - (32 + 8), NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));
}

void test_FreeOutsideHeap(void)
{
  struct kmalloc_state state;
    void *mem;
    __KMALlOC_SET_ABORT_CALLBACK(callback_abort);
    kmalloc_init((void *)heap, 512);
    mem = kmalloc(32);

    kfree(&heap[512]);

    TEST_ASSERT_EQUAL(1, cb_abort);

    state = kmalloc_debug_getstate();

    test_chunkinfo expected[2] = {
        {USED, 32 + 8, mem},
        {TC, 512 - DUMMYSIZE - (32 + 8), NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));
}

void test_FreeUnusedChunk(void)
{
  struct kmalloc_state state;
    void *mem1, *mem2;
    __KMALlOC_SET_ABORT_CALLBACK(callback_abort);
    kmalloc_init((void *)heap, 512);
    mem1 = kmalloc(32);
    mem2 = kmalloc(32);
    kfree(mem1);

    kfree(mem1);

    TEST_ASSERT_EQUAL(1, cb_abort);

    state = kmalloc_debug_getstate();

    test_chunkinfo expected[3] = {
        {FREE, 32 + 8, NULL},
        {USED, 32 + 8, mem2},
        {TC, 512 - DUMMYSIZE - (64 + 16), NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 3, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));
}

void test_FreeWrongPointer(void)
{
   struct kmalloc_state state;
    uint8_t *mem;
    __KMALlOC_SET_ABORT_CALLBACK(callback_abort);
    kmalloc_init((void *)heap, 512);
    mem = kmalloc(32);

    kfree(&mem[1]);

    TEST_ASSERT_EQUAL(1, cb_abort);

    state = kmalloc_debug_getstate();

    test_chunkinfo expected[2] = {
        {USED, 32 + 8, mem},
        {TC, 512 - DUMMYSIZE - (32 + 8), NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, 512));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, 512));
    TEST_ASSERT_FALSE(check_bins(heap,  512, state));
}


void test_FreeCorruption(void)
{
    uint8_t *mem1, *mem2;
    kmchunk_ptr c = (kmchunk_ptr) heap;


    __KMALlOC_SET_ABORT_CALLBACK(callback_abort);
    kmalloc_init((void *)heap, 512);
    mem1 = kmalloc(32);
    mem2 = kmalloc(32);
    kfree(mem1);

    c->header = 0xF0;
    kfree(mem2);

    TEST_ASSERT_EQUAL(1, cb_abort);

}

void test_FreeSchrinkTop(void)
{
    struct kmalloc_state state;
    void *mem;
    __KMALlOC_SET_SBRK_CALLBACK(callback_ok);
    kmalloc_init((void *)heap, HEAP_SHRINK_MIN);

    mem = kmalloc(HEAP_SHRINK_TRESHOLD + HEAP_SHRINK_MIN - DUMMYSIZE + 4);
    memset(mem, '#', HEAP_SHRINK_TRESHOLD + HEAP_SHRINK_MIN - DUMMYSIZE+ 4);


    kfree(mem);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(2, cb_ok);
    TEST_ASSERT_EQUAL(-HEAP_SHRINK_TRESHOLD - 8, increment);
    TEST_ASSERT_EQUAL(HEAP_SHRINK_MIN, state.heap_size);

    test_chunkinfo expected[1] = {
        {TC, HEAP_SHRINK_MIN - DUMMYSIZE, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, HEAP_SHRINK_MIN));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 1, state, heap, HEAP_SHRINK_MIN));
    TEST_ASSERT_FALSE(check_bins(heap,  HEAP_SHRINK_MIN, state));
}

void test_FreeSchrinkEmptyTop(void)
{
    struct kmalloc_state state;
    void *mem1, *mem2;
    __KMALlOC_SET_SBRK_CALLBACK(callback_ok);
    kmalloc_init((void *)heap, HEAP_SHRINK_MIN);

    mem1 = kmalloc(HEAP_SHRINK_MIN -8 - DUMMYSIZE + 4);
    memset(mem1, 'A', HEAP_SHRINK_MIN -8 - DUMMYSIZE + 4);
    mem2 = kmalloc(HEAP_SHRINK_TRESHOLD + 4);

    kfree(mem2);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(2, cb_ok);
    TEST_ASSERT_EQUAL(-HEAP_SHRINK_TRESHOLD - 8, increment);
    TEST_ASSERT_EQUAL(HEAP_SHRINK_MIN, state.heap_size);

    test_chunkinfo expected[1] = {
        {USED, HEAP_SHRINK_MIN - DUMMYSIZE, mem1}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, HEAP_SHRINK_MIN));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 1, state, heap, HEAP_SHRINK_MIN));
    TEST_ASSERT_FALSE(check_bins(heap,  HEAP_SHRINK_MIN, state));
}

void test_FreeSchrinkTop_heapsize_below_threshold(void)
{
    struct kmalloc_state state;
    void *mem1, *mem2;
    __KMALlOC_SET_SBRK_CALLBACK(callback_ok);
    kmalloc_init((void *)heap, HEAP_SHRINK_MIN);

    mem1 = kmalloc(64 + 4);
    memset(mem1, 'A', 64 + 4);
    mem2 = kmalloc(HEAP_SHRINK_TRESHOLD + 8);
    memset(mem2, 'B', HEAP_SHRINK_TRESHOLD + 8);

    kfree(mem2);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(1, cb_ok);
    TEST_ASSERT_EQUAL(64 + 8 + HEAP_SHRINK_TRESHOLD + 16 + DUMMYSIZE , state.heap_size);

    test_chunkinfo expected[2] = {
        {USED, 64 + 8, mem1},
        {TC, HEAP_SHRINK_TRESHOLD + 16, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, state.heap_size));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, state.heap_size));
    TEST_ASSERT_FALSE(check_bins(heap,  state.heap_size, state));
}

void test_FreeSchrinkTop_topchunk_below_treshold(void)
{
    struct kmalloc_state state;
    void *mem1, *mem2;
    __KMALlOC_SET_SBRK_CALLBACK(callback_ok);
    kmalloc_init((void *)heap, HEAP_SHRINK_MIN);

    mem1 = kmalloc(HEAP_SHRINK_MIN + 256);
    memset(mem1, 'A', HEAP_SHRINK_MIN + 256);
    mem2 = kmalloc(HEAP_SHRINK_TRESHOLD -16);
    memset(mem2, 'B', HEAP_SHRINK_TRESHOLD -16);

    kfree(mem2);

    state = kmalloc_debug_getstate();
    TEST_ASSERT_EQUAL(2, cb_ok);
    TEST_ASSERT_EQUAL(HEAP_SHRINK_MIN + 256 + 8 + HEAP_SHRINK_TRESHOLD - 16 + 8 + DUMMYSIZE , state.heap_size);

    test_chunkinfo expected[2] = {
        {USED, HEAP_SHRINK_MIN + 256 + 8, mem1},
        {TC, HEAP_SHRINK_TRESHOLD - 16 + 8, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrity(heap, state.heap_size));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, state.heap_size));
    TEST_ASSERT_FALSE(check_bins(heap,  state.heap_size, state));
}