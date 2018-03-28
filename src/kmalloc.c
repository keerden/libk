#include <stdio.h>
#include <stdint.h>
#include "kmalloc.h"
#include "kmalloc_util.h"

#include <string.h>





static void* kmalloc_chunk_frombin(int bin);
static void kmalloc_chunk_tobin(kmchunk_ptr chunk);
static void* kmalloc_split_chunk_frombin(int bin, size_t size);





void hexDump(char *desc, void *addr, int len) ;


static struct kmalloc_state kmstate;


struct kmalloc_state kmalloc_debug_getstate(void){
    return kmstate;
}

/*int mmain()
{
   char* test, *test2, *test3;
   printf("-- Initialazing heap\n");
   kmalloc_init((void *) heap, HEAPSIZE);
   test = kmalloc(24 * sizeof(char));
   strcpy(test, "(Test 1234567890 acdef)");
   test2 = kmalloc(24 * sizeof(char));
   strcpy(test2, "(%%%%%%%%%%%%%%%%%%%%%)");
   kfree(test2);
   kfree(test);
   printf("-- Bins \n");
   for(int i = 0; i < SBIN_COUNT; i++)
    {;
        printf("\t bin[%d] size: %d,  pointer: %08X \n", i, i * CHUNK_ALIGN_ON + MINCHUNKSIZE,(int) kmstate.bin[i] );
    }
   

   hexDump("-- Heap:", heap, HEAPSIZE);


   return 0;
}*/

void kmalloc_init(void *heap_addr, size_t heap_size)
{
    kmchunk_ptr chunk;
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
    kmstate.bin[i] = (i == calc_bin_no(chunksize))? chunk : NULL;
}



//init dummy end 'chunk' 
    chunk = (kmchunk_ptr) ((uint8_t*)chunk + chunksize);
    chunk->prev_foot = chunksize;
    chunk->header = 0;

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

    void* result;
    kmchunk_ptr chunk;

    // calculate chunksize and find suitable bin
    size_t wanted_size = calc_chunksize(size);

    if(wanted_size > SBIN_THRESHOLD)     //simple implementation for now: only support small bins
        return NULL;



    int bin = calc_bin_no(wanted_size);
    chunk = kmstate.bin[bin];
    
    if(chunk != NULL){
        result = kmalloc_chunk_frombin(bin);
    }else {
        while(bin < SBIN_COUNT && chunk == NULL){
            bin++;
            chunk = kmstate.bin[bin];
        }

        if(chunk != NULL) {
            result = kmalloc_split_chunk_frombin(bin, wanted_size);
        } else {
            //out of memory
            result = NULL;
        }

    }
   return result;
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
        kmalloc_dllist_remove_intern(coalescptr, &kmstate.bin[bin]);

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
            kmalloc_dllist_remove_intern(nextchunk, &kmstate.bin[bin]);
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

}

/****
 * static void* kmalloc_chunk_frombin(int bin)
 * 
 * Removes first chunk from bin list and mark it as used 
 * 
 * Input:
 *      int bin:  Number of bin to take a chunk from
 * 
 * Returns: 
 *      A pointer to the payload of the chunk, or NULL if failure.
 *****/

static void* kmalloc_chunk_frombin(int bin) 
{
    kmchunk_ptr chunk;
    kmchunk_ptr nextchunk;
    
    if(bin < 0 || bin >= SBIN_COUNT)
        return NULL;

    //remove chunk from list
    chunk = kmalloc_dllist_remove(&(kmstate.bin[bin]));
    
    if(chunk == NULL)
        return NULL;

    nextchunk = (kmchunk_ptr) ((uint8_t*) chunk + GETCHUNKSIZE(chunk->header));
    if(nextchunk->header != 0) nextchunk->header |= PINUSE;
    chunk->header |= CINUSE;

    return CHUNK_PAYLOAD(chunk);
}



/****
 * static void* kmalloc_split_chunk_frombin(int bin, size_t size)
 * 
 * Removes first chunk from bin list, splits it according to given size
 * allocates the first part an adds the last part to the right bin.
 * When size of the  last part is less than the minimum chunk size, 
 * no split is performed.
 * 
 * Input:
 *      int bin:  Number of bin to take a chunk from
 *      size_t size:  size of the requested chunk. 
 * 
 * Returns: 
 *      A pointer to the payload of the chunk, or NULL after failure.
 *****/

static void* kmalloc_split_chunk_frombin(int bin, size_t size) 
{
    kmchunk_ptr chunk;
    kmchunk_ptr newchunk;
    kmchunk_ptr nextchunk;

    size_t newsize;


    if(bin < 0 || bin >=SBIN_COUNT )
        return NULL;
    
    if(kmstate.bin[bin] == NULL)
        return NULL;
    
    size = chunksize_round(size);
    chunk = kmstate.bin[bin];

    if (GETCHUNKSIZE(chunk->header) < size)  //chunk is somehow too small, this should not happen
        return NULL;

    newsize = GETCHUNKSIZE(chunk->header) - size;
    
    if (newsize < MINCHUNKSIZE)         //if remainder is too small, just allocate whole chunk
        return kmalloc_chunk_frombin(bin);

   
    //remove chunk from list

    if(kmalloc_dllist_remove(&kmstate.bin[bin]) == NULL)
        return NULL;

    //split
    newchunk = (kmchunk_ptr) ((uint8_t*) chunk + size);
    newchunk->header = newsize |= PINUSE;

    //modify size for next chunk
    nextchunk = (kmchunk_ptr) ((uint8_t*) chunk + GETCHUNKSIZE(chunk->header));
    nextchunk->prev_foot = newsize;

    //update current current chunk
    chunk->header = size | PINUSE | CINUSE;  //chunk was free, this implies previous was used

    //add new chunk to correct freelist
    kmalloc_chunk_tobin(newchunk);

    return CHUNK_PAYLOAD(chunk);


}




static void kmalloc_chunk_tobin(kmchunk_ptr chunk)
{
    int bin = calc_bin_no(GETCHUNKSIZE(chunk->header));

    kmalloc_dllist_add(chunk, &kmstate.bin[bin]);

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