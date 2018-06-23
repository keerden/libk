#include <stdio.h>
#include <stdint.h>
#include "kmalloc.h"
#include "kmalloc_util.h"

#include <string.h>


//void hexDump(char *desc, void *addr, int len) ;


static struct kmalloc_state kmstate;

static void* allocate_sbin(binmap_t size);
static void* split_sbin(binmap_t index, size_t chunksize);
static void* allocate_dv(size_t chunksize);
static void* allocate_smallest_tbin(size_t chunksize);
static void* split_top(size_t chunksize);
static void replace_dv(kmchunk_ptr new);




struct kmalloc_state kmalloc_debug_getstate(void){
    return kmstate;
}


void kmalloc_init(void *heap_addr, size_t heap_size)
{
    kmchunk_ptr top_chunk;
    size_t top_size, offset;
    
    if(heap_addr == NULL)
        return;


    top_chunk = CHUNKALIGN(heap_addr);
    offset = (size_t )top_chunk - (size_t ) heap_addr;


    if((offset + MINCHUNKSIZE + DUMMYSIZE) > heap_size)
        return;

    top_size = CHUNKFLOOR(heap_size - offset - DUMMYSIZE);


    top_chunk->header = top_size | PINUSE;
    top_chunk->next = top_chunk->prev = top_chunk;


    //set dummy
    NEXTCHUNK(top_chunk)->prev_foot = top_size;
    NEXTCHUNK(top_chunk)->header = 0;

    kmstate.heap_start = top_chunk;
    kmstate.heap_size = heap_size - offset;
    kmstate.sbinmap = kmstate.tbinmap = 0;
    for(size_t i = 0; i < NSBINS; i++)
        kmstate.sbin[i] = NULL;
    for(size_t i = 0; i < NTBINS; i++)
        kmstate.tbin[i] = NULL;
    kmstate.dVictim = NULL;
    kmstate.dVictimSize = 0;
    kmstate.topChunk = top_chunk;
    kmstate.topChunkSize = top_size;
    kmstate.magic = KMALLOC_STATE_MAGIC;
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
    size_t chunksize;
    binmap_t index, bits;
    void* result = NULL;


    if(!KMALLOC_IS_INIT(kmstate))
        return NULL;
    if(REQUESTOVERFLOW(size))
        return NULL;

    chunksize = REQUEST2SIZE(size);

    if(chunksize < MIN_LARGE_SIZE){ // use small bins
        index = small_index(chunksize);
        bits = (kmstate.sbinmap >> index) & 0x3U;  
        if(bits){   //check if there could be a remainderless fit
            index += ~bits & 1;     //increase index if exact matching bin is empty
            result = allocate_sbin(index);
        } else if(chunksize <= kmstate.dVictimSize){
            //use dv
            result = allocate_dv(chunksize);
        } else {
            //find a suitable bin, or use top
            index += 2;
            bits = kmstate.sbinmap >> index;
            while(bits && !(bits & 1)){
                index++;
                bits >>=1;
            }
            if(bits){
                result = split_sbin(index, chunksize);
            } else if(kmstate.tbinmap){
                result = allocate_smallest_tbin(chunksize);
            } else {
                result = split_top(chunksize);
            }
        }

    }else{  //use tree bins


    }

    if(result == NULL){
        //handle out of mem
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
    kmchunk_ptr freeptr, prev, next, chunk = PAYLOAD_CHUNK(ptr);   

    size_t freesize, csize, index;

    if(!ADDRESS_OK(chunk, kmstate) || !IS_INUSE(chunk)) 
    {
        return;
    }
    
    freeptr = chunk;
    freesize = GETCHUNKSIZE(chunk);

    if(!PREV_INUSE(chunk))
    {
        csize = chunk->prev_foot;
        prev = PREVCHUNK(chunk);
        if(!ADDRESS_OK(prev, kmstate) ||  GETCHUNKSIZE(prev) != csize ||  IS_INUSE(prev)) {
            return;
        }
        freeptr = prev;
        freesize += csize;

        if(kmstate.dVictim != prev){    //if not dv, remove prev from bin
            if(csize < MIN_LARGE_SIZE){ 
                index = small_index(csize);
                kmalloc_dllist_remove(&kmstate.sbin[index]);
                if(kmstate.sbin[index] == NULL)
                    kmstate.sbinmap &= ~(SIZE_T_ONE << index);

            } else {
                //treebins
            }
        }

    }
    next = NEXTCHUNK(chunk);

    if(!ADDRESS_OK(next, kmstate))
        return;

    if(!IS_INUSE(next)){
        csize = GETCHUNKSIZE(next);
        freesize += csize;
        freeptr->header = freesize | PREV_INUSE(freeptr);
        CHUNKOFFSET(freeptr, freesize)->prev_foot = freesize;
        if(next == kmstate.topChunk){
            if(freeptr == kmstate.dVictim){
                kmstate.dVictim = NULL;
                kmstate.dVictimSize = 0;
            }
            kmstate.topChunk = freeptr;
            kmstate.topChunkSize = freesize;
        } else if(next == kmstate.dVictim){
            kmstate.dVictim = freeptr;
            kmstate.dVictimSize = freesize;
        } else {
            //unlink
            if(csize < MIN_LARGE_SIZE){ 
                index = small_index(csize);
                kmalloc_dllist_remove(&kmstate.sbin[index]);
                if(kmstate.sbin[index] == NULL)
                    kmstate.sbinmap &= ~(SIZE_T_ONE << index);
            } else {
                //treebins
            }

            if(freeptr == kmstate.dVictim){
                kmstate.dVictim = freeptr;
                kmstate.dVictimSize = freesize;
            }else{  //add consolidated chunk to sbin
                if(freesize < MIN_LARGE_SIZE){
                    index = small_index(freesize);
                    kmalloc_dllist_add(freeptr, &kmstate.sbin[index]);
                     kmstate.sbinmap |= (1 << index);
                } else {
                    //TODO tbins
                }
            }
        }
    } else {
        freeptr->header = freesize | PREV_INUSE(freeptr);
        next = CHUNKOFFSET(freeptr, freesize);
        next->prev_foot = freesize;
        next->header &= ~PINUSE;
        if(freeptr == kmstate.dVictim){
            kmstate.dVictim = freeptr;
            kmstate.dVictimSize = freesize;
        }else{  //add consolidated chunk to sbin
            if(freesize < MIN_LARGE_SIZE){
                index = small_index(freesize);
                kmalloc_dllist_add(freeptr, &kmstate.sbin[index]);
                    kmstate.sbinmap |= (1 << index);
            } else {
                //TODO tbins
            }
        }        
    }
}


static void* allocate_sbin(binmap_t index) {

    kmchunk_ptr chunk = kmalloc_dllist_remove(&kmstate.sbin[index]);
    if(kmstate.sbin[index] == NULL)
        kmstate.sbinmap &= ~(SIZE_T_ONE << index);
    if(chunk == NULL)
        return NULL;

    chunk->header |= CINUSE;
    NEXTCHUNK(chunk)->header |= PINUSE;
    return CHUNK_PAYLOAD(chunk);
}

static void* split_sbin(binmap_t index, size_t chunksize) {
    
    kmchunk_ptr chunk, new; 
    size_t split_size;

    chunk = kmalloc_dllist_remove(&kmstate.sbin[index]);

    if(kmstate.sbin[index] == NULL)
        kmstate.sbinmap &= ~(SIZE_T_ONE << index);
    if(chunk == NULL)
        return NULL;    
    
    split_size =  GETCHUNKSIZE(chunk) - chunksize;
    chunk->header = chunksize | PINUSE | CINUSE;    //previous chunk cannot be a free chunk, so PINUSE is set
  
    new = CHUNKOFFSET(chunk, chunksize);
    new->header = split_size | PINUSE;
    NEXTCHUNK(new)->prev_foot = split_size;

    replace_dv(new);

    return CHUNK_PAYLOAD(chunk);;
}

static void* allocate_dv(size_t chunksize){
    kmchunk_ptr chunk = NULL;
    size_t split_size;

    if(kmstate.dVictim == NULL || kmstate.dVictimSize < chunksize)
        return NULL;

    if(kmstate.dVictimSize - chunksize < MINCHUNKSIZE) { //exhaust dv
        chunk = kmstate.dVictim;
        chunk->header |= CINUSE;
        NEXTCHUNK(chunk)->header |= PINUSE;
        kmstate.dVictim = NULL;
        kmstate.dVictimSize = 0;
    }else{  //split dv
        split_size = kmstate.dVictimSize - chunksize;
        chunk = kmstate.dVictim;
        chunk->header = chunksize | PINUSE | CINUSE;    //previous chunk cannot be a free chunk, so PINUSE is set
        kmstate.dVictim = CHUNKOFFSET(chunk, chunksize);
        kmstate.dVictim->header = split_size | PINUSE;
        kmstate.dVictimSize = split_size;
        NEXTCHUNK(kmstate.dVictim)->prev_foot = split_size;
    }
    return CHUNK_PAYLOAD(chunk);
}

static void replace_dv(kmchunk_ptr new){
    size_t index;

    if(kmstate.dVictim != NULL){
        if(kmstate.dVictimSize < MIN_LARGE_SIZE){
            index = small_index(kmstate.dVictimSize);
            kmalloc_dllist_add(new, &kmstate.sbin[index]);
            kmstate.sbinmap |= (1 << index);
        } else {
            //TODO tbins
        }
    }

    kmstate.dVictim = new;
    kmstate.dVictimSize = GETCHUNKSIZE(new);

}



static void* allocate_smallest_tbin(size_t chunksize) {
    return NULL;
}

static void* split_top(size_t chunksize) {
    kmchunk_ptr chunk = NULL;
    size_t split_size;

    if(kmstate.topChunk == NULL || kmstate.topChunkSize < chunksize)
        return NULL;

    if(kmstate.topChunkSize - chunksize < MINCHUNKSIZE) { //exhaust top
        chunk = kmstate.topChunk;
        chunk->header |= CINUSE;
        kmstate.topChunk = NEXTCHUNK(chunk);
        kmstate.topChunkSize = 0;
    }else{  //split top
        split_size = kmstate.topChunkSize - chunksize;
        chunk = kmstate.topChunk;
        chunk->header = chunksize | PINUSE | CINUSE;    //previous chunk cannot be a free chunk, so PINUSE is set
        kmstate.topChunk = CHUNKOFFSET(chunk, chunksize);
        kmstate.topChunk->header = split_size | PINUSE;
        kmstate.topChunkSize = split_size;
        NEXTCHUNK(kmstate.topChunk)->prev_foot = split_size;
    }
    return CHUNK_PAYLOAD(chunk);
}

