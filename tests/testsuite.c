#include "testsuite.h"
#include "kmalloc.h"
#include "kmalloc_util.h"
#include <stdio.h>

#define REPORT(str, addr, heap) {printf("Error at chunk: %08x -- %s\n", (int) (addr) - (int) (heap), (str)); return 1;}




/***
 * int check_heap_integrety(uint8_t *heap, size_t heapsize)
 * 
 * Walks through the heap and checks if all chunks are consistent
 * Free chunks should never follow another free chunk
 * 
 * returns 0 when no errors where found
 ***/


int check_heap_integrety(void *heap, size_t heapsize) {

    kmchunk_ptr chunk = (kmchunk_ptr) heap;
    int prev_inuse = 1; //first chunk should always indicate PINUSE

    size_t prev_size, size;

    while(chunk->header != 0) {

        size = GETCHUNKSIZE(chunk);
        if(((size_t) chunk + size) > ((size_t) heap + heapsize)) REPORT("Chunksize overflows heap", chunk, heap);
        if(size < MINCHUNKSIZE) REPORT("Chunksize too small", chunk, heap);
        if(size % CHUNK_ALIGN_ON) REPORT("Chunksize not aligned", chunk, heap);

        if (prev_inuse) {
            if(!(chunk->header & PINUSE)) REPORT("PINUSE not set", chunk, heap);
        } else {
            if((chunk->header & PINUSE)) REPORT("PINUSE is set", chunk, heap);
            if((chunk->prev_foot !=  prev_size)) REPORT("prev_foot doesn't match size of previous chunk", chunk, heap);
            if(!(chunk->header & CINUSE)) REPORT("A free chunk should never follow another free chunk", chunk, heap);
        }

        prev_size = size;
        prev_inuse = chunk->header & CINUSE;
        chunk = (kmchunk_ptr) ((size_t) chunk + size);
    }

    if(prev_inuse && (chunk->prev_foot !=  prev_size)) REPORT("prev_foot of dummy chunk doesn't match size of previous chunk", chunk, heap);
    if(((size_t) chunk + 8) > ((size_t) heap + heapsize)) REPORT("dummy chunk overflows heap", chunk, heap);
    if(((size_t) chunk + 8) < ((size_t) heap + heapsize)) REPORT("dummy chunk not placed at end of heap", chunk, heap);
    return 0;
}



/***
 * int check_heap_layout(test_chunkinfo expected, struct kmalloc_state kmstate,uint8_t heap, size_t heapsize)
 * 
 * Walks through the heap and checks if all chunks are consistent with given layout
 * kmstate is checked in order to verify if a chunk is a TC or DV
 * also each free chunk (except TC or DV) must be present in a bin list
 * 
 * returns 0 when no errors where found
 ***/


int check_heap_layout   (  test_chunkinfo *expected, 
                            int testsize, 
                            struct kmalloc_state kmstate,
                            void *heap, 
                            size_t heapsize
                        )
{
    int index, binindex;
    size_t size;
    kmchunk_ptr chunk = (kmchunk_ptr) heap;
    kmchunk_ptr listchunk, liststart;
    test_chunkinfo test;

    for(index = 0; index < testsize; index++) {
        test = expected[index]; 
        size = GETCHUNKSIZE(chunk);
        if(test.size != size) REPORT("Chunksize mismatch", chunk, heap);
        if((size_t)chunk + size > (size_t) heapsize) REPORT("Chunk overflows heap", chunk, heap);

        if(test.type == USED) {
            if(!(chunk->header & CINUSE)) REPORT("Chunk should be in use", chunk, heap);
            if(test.payload != NULL && test.payload != CHUNK_PAYLOAD(chunk)) REPORT("Payload address mismatch", chunk, heap);
        } else {
            if(chunk->header & CINUSE) REPORT("Chunk should not be in use", chunk, heap);

            switch (test.type){
                case FREE:
                    //check small or treebin
                    if(size <= MAX_SMALL_SIZE) {
                        binindex = small_index(size);
                        if(!(kmstate.sbinmap & (SIZE_T_ONE << binindex))) REPORT("Unused small chunk not present in sbinmap", chunk, heap);
                        if(kmstate.sbin[binindex] == NULL) REPORT("Empty sbin", chunk, heap);
                        liststart = listchunk = kmstate.sbin[binindex];
                        while(listchunk != chunk){
                            listchunk = listchunk->next;
                            if(listchunk == liststart) 
                                break;
                        }
                        if(listchunk != chunk) REPORT("Unused small chunk not present in sbin list", chunk, heap);
                    } else {

                    }
                    break;

                case TC: 
                    if(kmstate.topChunkSize != size) REPORT("Topchunk size mismatch in kmstate", chunk, heap);
                    if(kmstate.topChunk != chunk) REPORT("Topchunk pointer mismatch in kmstate", chunk, heap);
                    break;
                case DV:
                    if(kmstate.dVictimSize != size) REPORT("dVictim size mismatch in kmstate", chunk, heap);
                    if(kmstate.dVictim!= chunk) REPORT("dVictim pointer mismatch in kmstate", chunk, heap);
                    break;
                default:
                    REPORT("Unknown type specified in test_chunkinfo", chunk, heap);
                    break;
            }
        }
        chunk = (kmchunk_ptr) ((size_t) chunk + size);

    }

    return 0;
}

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
