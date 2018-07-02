#ifndef _KSTRING_H
#define _KSTRING_H 1

#include <kstddef.h>


#ifdef __cplusplus
extern "C" {
#endif

//copying functions

void *kmemcpy(void * __restrict s1, const void * __restrict s2, size_t n);
void *kmemmove(void *s1, const void *s2, size_t n);
char *kstrcpy(char * __restrict s1, const char * __restrict s2);
char *kstrncpy(char * __restrict s1, const char * __restrict s2, size_t n);

//Concatenation functions

char *kstrcat(char * __restrict s1, const char * __restrict s2);
char *kstrncat(char * __restrict s1, const char * __restrict s2, size_t n);

//comparison functions

int kmemcmp(const void *s1, const void *s2, size_t n);
int kstrcmp(const char *s1, const char *s2);


//search functions

// misc functions
void *kmemset(void *s, int c, size_t n);
char *kstrerror(int errnum);
size_t kstrlen(const char *s);


#ifdef __cplusplus
}
#endif

#endif
