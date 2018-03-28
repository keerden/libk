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

typedef struct kmalloc_chunk kmchunk;
typedef struct kmalloc_chunk* kmchunk_ptr;


#define PINUSE 1
#define CINUSE 2
#define FLAGMASK 0x00000007

#define GETCHUNKSIZE(x) (x & ~FLAGMASK)

#define CHUNK_PAYLOAD(chunk)    ((void *) ((uint8_t *) chunk + offsetof(struct kmalloc_chunk, next)))
#define PAYLOAD_CHUNK(ptr)      ((kmchunk_ptr) ((uint8_t *) ptr - offsetof(struct kmalloc_chunk, next)))

#define CHUNK_OVERHEAD (sizeof(size_t))
#define CHUNK_ALIGN_ON        8
#define MINCHUNKSIZE          16
#define SBIN_THRESHOLD        256

#define CHUNKALIGN(x)         (((x) + CHUNK_ALIGN_ON - 1) & ~(CHUNK_ALIGN_ON - 1))
#define CHUNKFLOOR(x)         ((x) & ~(CHUNK_ALIGN_ON - 1))

#define SBIN_COUNT ((SBIN_THRESHOLD - MINCHUNKSIZE) / CHUNK_ALIGN_ON) + 1


struct kmalloc_tree_chunk {
    size_t prev_foot;
    size_t header;
    struct kmalloc_tree_chunk*   next;
    struct kmalloc_tree_chunk*   prev;

    struct kmalloc_tree_chunk*   left;
    struct kmalloc_tree_chunk*   right;
    struct kmalloc_tree_chunk*   parent;
};

typedef struct kmalloc_tree_chunk ktchunk;
typedef struct kmalloc_tree_chunk* ktchunk_ptr;








struct kmalloc_state {
    void *heap_start;
    size_t heap_size;
    kmchunk_ptr bin[SBIN_COUNT];
};


void kmalloc_init(void *heap_addr, size_t heap_size);
void* kmalloc (size_t size);
void kfree(void *ptr);

struct kmalloc_state kmalloc_debug_getstate(void);

#define chunksize_round(size) ((size < MINCHUNKSIZE)? MINCHUNKSIZE : CHUNKALIGN(size))
#define calc_chunksize(size) chunksize_round(size + CHUNK_OVERHEAD)
#define calc_bin_no(size) ((size < MINCHUNKSIZE)? 0 : ((size - MINCHUNKSIZE) / CHUNK_ALIGN_ON))


#endif // !__KMALLOC

