#include <stdio.h>
#include <stdint.h>
#include "kmalloc.h"
#include "kmalloc_util.h"

#include <string.h>





void hexDump(char *desc, void *addr, int len) ;


static struct kmalloc_state kmstate;


struct kmalloc_state kmalloc_debug_getstate(void){
    return kmstate;
}


void kmalloc_init(void *heap_addr, size_t heap_size)
{
/*    kmchunk_ptr chunk;
    size_t chunksize;
    
//init kmstate
    
    
    kmstate.heap_size = heap_size;
    kmstate.heap_start = heap_addr;

    

//init empty topchunk 

    chunk = (kmchunk_ptr) heap_addr;
    chunksize = CHUNKFLOOR(heap_size - 1);

//todo: largebins
if(chunksize > SBIN_THRESHOLD)
    chunksize = SBIN_THRESHOLD;

    chunk->header = chunksize | PINUSE;
    chunk->prev = chunk;
    chunk->next = chunk;

//init bins
for(size_t i = 0; i < SBIN_COUNT; i++)
{
    kmstate.sbin[i] = (i == calc_bin_no(chunksize))? chunk : NULL;
}



//init dummy end 'chunk' 
    chunk = (kmchunk_ptr) ((uint8_t*)chunk + chunksize);
    chunk->prev_foot = chunksize;
    chunk->header = 0;
*/
}


/****
 * void* kmalloc (size_t size)
 * 
 * Allocates a block of at least 'size' bytes on heap. 
 * 
 * Input:
 *      size_t size:  Size of block to allocate. Could also be zero
 * 
 * Returns: 
 *      A pointer to the block of memory or NULL if there was a failure
 *****/

void* kmalloc (size_t size) 
{
/*
    void* result;
    kmchunk_ptr chunk;

    // calculate chunksize and find suitable bin
    size_t wanted_size = calc_chunksize(size);

    if(wanted_size > SBIN_THRESHOLD)     //simple implementation for now: only support small bins
        return NULL;



    int bin = calc_bin_no(wanted_size);
    chunk = kmstate.sbin[bin];
    
    if(chunk != NULL){
        result = kmalloc_chunk_frombin(bin);
    }else {
        while(bin < SBIN_COUNT && chunk == NULL){
            bin++;
            chunk = kmstate.sbin[bin];
        }

        if(chunk != NULL) {
            result = kmalloc_split_chunk_frombin(bin, wanted_size);
        } else {
            //out of memory
            result = NULL;
        }

    }
   return result;
   */
}


/****
 * void kfree(void *ptr) 
 * 
 * Returns a block to the free list.
 * 
 * Input:
 *      void *ptr, pointer to allocated block
 * 
 * 
 *****/

void kfree(void *ptr) 
{
    /*
    kmchunk_ptr coalescptr, nextchunk;
    size_t newsize, cursize;
    int bin;
    kmchunk_ptr chunk = PAYLOAD_CHUNK(ptr);
    if(((void*) chunk) < kmstate.heap_start)    //no heap address
        return;
    if(((void *) chunk ) >= ((void *) ( (uint8_t*)kmstate.heap_start + kmstate.heap_size))) //pointer past end of heap
        return;
    
    if(!(chunk->header & CINUSE)) //chunk not in use
        return;

    newsize = GETCHUNKSIZE(chunk->header);
    //check free block before
    if(!(chunk->header & PINUSE)){  
        cursize = chunk->prev_foot;
        coalescptr = (kmchunk_ptr) ((uint8_t*) chunk - cursize);
        bin =  calc_bin_no(cursize);  
        
        //remove chunk from list
        kmalloc_dllist_remove_intern(coalescptr, &kmstate.sbin[bin]);

        newsize += cursize;
    } else{
        coalescptr = chunk;
    }

    //check free block after chunk
    nextchunk = (kmchunk_ptr) ((uint8_t*) chunk + GETCHUNKSIZE(chunk->header));

    if(!(nextchunk->header & CINUSE) && nextchunk->header != 0)   //if next is not in use, and not the end chunk
    {
            cursize = GETCHUNKSIZE(nextchunk->header);
            bin =  calc_bin_no(cursize);  
            
            //remove chunk from list
            kmalloc_dllist_remove_intern(nextchunk, &kmstate.sbin[bin]);
            newsize += cursize;       
    }

    coalescptr->header = newsize | PINUSE;
    nextchunk = (kmchunk_ptr) ((uint8_t*) coalescptr + newsize); 
    //set header and size in next block
    nextchunk->prev_foot = newsize;
    if(nextchunk->header != 0)
        nextchunk->header = nextchunk->header & ~PINUSE;
    //add to correct bin and list
    kmalloc_chunk_tobin(coalescptr);
    */
}


void hexDump(char *desc, void *addr, int len) 
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