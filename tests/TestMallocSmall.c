#include "unity.h"
#include "kmalloc.h"
#include "kmalloc_util.h"
#include "testsuite.h"
#include <string.h>

#define HEAPSIZE 512

uint8_t heap[HEAPSIZE];

void setUp(void)
{
    memset(heap, 0, HEAPSIZE);
    kmalloc_init((void *)heap, HEAPSIZE);
}

void tearDown(void)
{
}

void test_InitialHeap(void)
{
    struct kmalloc_state state = kmalloc_debug_getstate();

    test_chunkinfo expected[1] = {
        {TC, HEAPSIZE - DUMMYSIZE, NULL}};
    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 1, state, heap, HEAPSIZE));
}

void test_SingleMalloc(void)
{
    struct kmalloc_state state;
    void *mem;

    mem = kmalloc(32 + 4);

    state = kmalloc_debug_getstate();

    test_chunkinfo expected[2] = {
        {USED, 32 + 8, mem},
        {TC, HEAPSIZE -DUMMYSIZE - (32 + 8), NULL}};

    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, HEAPSIZE));
}

void test_MallocNullSize(void)
{
    struct kmalloc_state state;
    void *mem;

    mem = kmalloc(0);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[2] = {
        {USED, 8 + 8, mem},
        {TC, HEAPSIZE - DUMMYSIZE - (8 + 8), NULL}};

    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 2, state, heap, HEAPSIZE));
}

void test_MultipleSmallMalloc(void)
{
    void *mem1, *mem2, *mem3, *mem4;
    struct kmalloc_state state;

    mem1 = kmalloc(16);
    mem2 = kmalloc(32);
    mem3 = kmalloc(64);
    mem4 = kmalloc(8);


    state = kmalloc_debug_getstate();
    test_chunkinfo expected[5] =    {
                                    {USED, 16 + 8, mem1},
                                    {USED, 32 + 8, mem2},
                                    {USED, 64 + 8, mem3},
                                    {USED, 8 + 8, mem4},
                                    {TC, HEAPSIZE - DUMMYSIZE - 4 * 8 - 120, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 5, state, heap, HEAPSIZE));

    kfree(mem1);
    kfree(mem2);
    kfree(mem3);
    kfree(mem4);

    //state should be the same as initial state
    test_InitialHeap();
}

void test_MallocOverLimit(void)  //allocate more than heapsize, but less than max mem
{
    TEST_IGNORE();
}

void test_MallocMaxSize(void)   //allocate all space within the heap
{
    TEST_IGNORE();
}

void test_MallocOutOfMem(void)  //allocate more than heapsize and more than max mem
{
    TEST_IGNORE();
}

void test_MallocSmallFromSBin(void)
{
    void *mem1, *mem2, *mem3;
    struct kmalloc_state state;

    mem1 = kmalloc(64);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(64);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[3] =   {{USED, 64 + 8, mem3},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 112, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 3, state, heap, HEAPSIZE));
}

void test_MallocSmallFromNextBin(void)
{
    void *mem1, *mem2, *mem3;
    struct kmalloc_state state;

    mem1 = kmalloc(64);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(56);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[3] =   {{USED, 64 + 8, mem3},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 112, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 3, state, heap, HEAPSIZE));
}
void test_MallocSmallFromLargerBin(void)
{
    void *mem1, *mem2, *mem3;
    struct kmalloc_state state;

    mem1 = kmalloc(64);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(32);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[4] =   {{USED, 32 + 8, mem3},
                                    {DV, 32, NULL},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 112, NULL}};;
    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 4, state, heap, HEAPSIZE));
}

void test_MallocSmallFromDvSplit(void)
{
    void *mem1, *mem2, *mem3, *mem4;
    struct kmalloc_state state;

    mem1 = kmalloc(64);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(32); //a split occurs resulting in a dv of 32
    mem4 = kmalloc(8);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[5] =   {{USED, 32 + 8, mem3},
                                    {USED, 8 + 8, mem4},
                                    {DV, 16, NULL},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 112, NULL}};
    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 5, state, heap, HEAPSIZE));
}

void test_MallocSmallFromDvExhaustFit(void)
{
    void *mem1, *mem2, *mem3, *mem4;
    struct kmalloc_state state;

    mem1 = kmalloc(64);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(32); //a split occurs resulting in a dv of 32
    mem4 = kmalloc(24);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[4] =   {{USED, 32 + 8, mem3},
                                    {USED, 24 + 8, mem4},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 112, NULL}};
    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 4, state, heap, HEAPSIZE));
}
void test_MallocSmallFromDvExhaustRemainder(void)
{
    void *mem1, *mem2, *mem3, *mem4;
    struct kmalloc_state state;

    mem1 = kmalloc(64);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(32); //a split occurs resulting in a dv of 32
    mem4 = kmalloc(16);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[4] =   {{USED, 32 + 8, mem3},
                                    {USED, 16 + 8 + 8, mem4},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 112, NULL}};
    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 4, state, heap, HEAPSIZE));
}

void test_MallocSmallFromTBin(void)
{
    void *mem1, *mem2, *mem3;
    struct kmalloc_state state;

    mem1 = kmalloc(256 - 8);
    mem2 = kmalloc(32);
    kfree(mem1);
    mem3 = kmalloc(64);

    state = kmalloc_debug_getstate();
    test_chunkinfo expected[4] =   {{USED, 64 + 8, mem3},
                                    {DV, 256 - 64 - 8, NULL},
                                    {USED, 32 + 8, mem2},
                                    {TC, HEAPSIZE - DUMMYSIZE - 296, NULL}};

    TEST_ASSERT_FALSE(check_heap_integrety(heap, HEAPSIZE));
    TEST_ASSERT_FALSE(check_heap_layout(expected, 4, state, heap, HEAPSIZE));
}


