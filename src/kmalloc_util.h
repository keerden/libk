#ifndef __KMALLOC_UTIL
#define __KMALLOC_UTIL

#include "kmalloc.h"





/****
 *  void kmalloc_dllist_add(kmchunk_ptr chunk, kmchunk_ptr head)
 * 
 *  Adds a chunk to the front of the list and modifies the head.
 *
 *  inputs
 *      kmchunk_ptr chunk  -  pointer to the new chunk to be added
 *      kmchunk_ptr *head  -  pointer to the headpointer
 * */
void kmalloc_dllist_add(kmchunk_ptr chunk, kmchunk_ptr *head);

/****
 *  kmchunk_ptr kmalloc_dllist_remove_intern(kmchunk_ptr chunk, kmchunk_ptr *head);
 * 
 *  Removes the given chunk from the list and updates the head if needed.
 *
 *  inputs
 *      kmchunk_ptr chunk  -  pointer to the chunk to be removed
 *      kmchunk_ptr *head  -  pointer to the headpointer
 *
 *  Returns:
 *      pointer to the removed chunk 
 * */

kmchunk_ptr kmalloc_dllist_remove_intern(kmchunk_ptr chunk, kmchunk_ptr *head);

/****
 *  kmchunk_ptr kmalloc_dllist_remove(kmchunk_ptr *head);
 * 
 *  Removes the chunk from the front of the list.
 *
 *  inputs
 *      kmchunk_ptr *head  -  pointer to the headpointer
 *
 *  Returns:
 *      pointer to the removed chunk 
 * */

kmchunk_ptr kmalloc_dllist_remove(kmchunk_ptr *head);


/****
 *  void kmalloc_chunk_iterate(kmchunk_ptr *chunk);
 * 
 *  Used for iterating through all chunks in heap.
 *
 *  inputs
 *      kmchunk_ptr *chunk  -  pointer to current chunk address
 *
 *  outputs
 *      kmchunk_ptr *chunk  -  points to next chunk, 
 *                             equal to NULL when the end of heap is reached
 * */

void kmalloc_chunk_iterate(kmchunk_ptr *chunk);


#endif // !__KMALLOC_UTIL
