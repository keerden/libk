#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

int kprintf(const char* __restrict, ...);
int kputchar(int);
int kputs(const char*);

#ifdef __cplusplus
}
#endif

#endif
