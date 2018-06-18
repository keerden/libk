#ifndef __KMALLOC
#define __KMALLOC

#include <stdint.h>
#include <stddef.h>



  


/***
 * Chunk definition
 */

struct kmalloc_chunk {
    size_t prev_foot;
    size_t header;
    struct kmalloc_chunk*   next;
    struct kmalloc_chunk*   prev;
};



struct kmalloc_tree_chunk {
    size_t prev_foot;
    size_t header;
    struct kmalloc_tree_chunk*   next;
    struct kmalloc_tree_chunk*   prev;

    struct kmalloc_tree_chunk*   left;
    struct kmalloc_tree_chunk*   right;
    struct kmalloc_tree_chunk*   parent;
};

typedef struct kmalloc_chunk kmchunk;
typedef struct kmalloc_chunk* kmchunk_ptr;
typedef struct kmalloc_tree_chunk ktchunk;
typedef struct kmalloc_tree_chunk* ktchunk_ptr;

typedef unsigned int binmap_t;


/******* size_t constants ********/
#define SIZE_T_ZERO         ((size_t)0)
#define SIZE_T_ONE          ((size_t)1)
#define SIZE_T_TWO          ((size_t)2)
#define SIZE_T_FOUR         ((size_t)4)
#define SIZE_T_SIZE         (sizeof(size_t))
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define FOUR_SIZE_T_SIZES   (SIZE_T_SIZE<<2)
#define SIX_SIZE_T_SIZES    (FOUR_SIZE_T_SIZES+TWO_SIZE_T_SIZES)
#define HALF_MAX_SIZE_T     (MAX_SIZE_T / 2U)


/****** chunks *******/

#define PINUSE      (0x1U)
#define CINUSE      (0x2U)
#define FLAGMASK    (0x00000007U)

#define CHUNK_OVERHEAD        (SIZE_T_SIZE)
#define CHUNK_ALIGN_ON        (8U)
#define CHUNK_ALIGN_MASK      (CHUNK_ALIGN_ON - 1)
#define MINCHUNKSIZE          (16U)
#define DUMMYSIZE             (CHUNK_ALIGN_ON)


#define GETCHUNKSIZE(c)         (((c)->header) & ~FLAGMASK)
#define CHUNK_PAYLOAD(chunk)    ((void *) ((uint8_t *) chunk + offsetof(struct kmalloc_chunk, next)))
#define PAYLOAD_CHUNK(ptr)      ((kmchunk_ptr) ((uint8_t *) ptr - offsetof(struct kmalloc_chunk, next)))
#define CHUNKALIGN(x)           (((x) + CHUNK_ALIGN_ON - 1) & ~CHUNK_ALIGN_MASK)
#define CHUNKFLOOR(x)           ((x) & ~CHUNK_ALIGN_MASK)

/******* Bin types, widths and sizes ********/
#define NSBINS            (32U)
#define NTBINS            (32U)
#define SBIN_SHIFT        (3U)
#define SBIN_WIDTH        (SIZE_T_ONE << SBIN_SHIFT)
#define TBIN_SHIFT        (8U)
#define MIN_LARGE_SIZE    (SIZE_T_ONE << TBIN_SHIFT)
#define MAX_SMALL_SIZE    (MIN_LARGE_SIZE - SIZE_T_ONE)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

/******* smallbins ************/
#define small_index(s)      ((s) >> SBIN_SHIFT)
#define small_index2size(i) ((i) << SBIN_SHIFT)
#define MIN_SMALL_INDEX     (small_index(MINCHUNKSIZE))

struct kmalloc_state {
    void *heap_start;
    size_t heap_size;
    binmap_t    sbinmap;
    binmap_t    tbinmap;
    kmchunk_ptr sbin[NSBINS];
    kmchunk_ptr tbin[NTBINS];
    kmchunk_ptr dVictim;
    size_t      dVictimSize;
    kmchunk_ptr topChunk;
    size_t      topChunkSize;
};


void kmalloc_init(void *heap_addr, size_t heap_size);
void* kmalloc (size_t size);
void kfree(void *ptr);

struct kmalloc_state kmalloc_debug_getstate(void);

#define chunksize_round(size) ((size < MINCHUNKSIZE)? MINCHUNKSIZE : CHUNKALIGN(size))
#define calc_chunksize(size) chunksize_round(size + CHUNK_OVERHEAD)
#define calc_bin_no(size) ((size < MINCHUNKSIZE)? 0 : ((size - MINCHUNKSIZE) / CHUNK_ALIGN_ON))


#endif // !__KMALLOC

