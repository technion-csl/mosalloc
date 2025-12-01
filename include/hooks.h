
#ifndef _MEM_ALLOC_HOOK_H_
#define _MEM_ALLOC_HOOK_H_

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
#define __THROW_EXCEPTION __THROW
#else
#define __THROW_EXCEPTION
#endif /* __cplusplus */

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

int mprotect(void *addr, size_t len, int prot) __THROW_EXCEPTION;
void *mmap(void *addr, size_t length, int prot, int flags, int fd, 
           off_t offset) __THROW_EXCEPTION;
int munmap(void *addr, size_t length) __THROW_EXCEPTION;

int brk(void *addr) __THROW_EXCEPTION;
void *sbrk(ptrdiff_t increment) __THROW_EXCEPTION;
void *mosalloc_morecore(ptrdiff_t increment) __THROW_EXCEPTION;
 
#ifdef __cplusplus
}  /* end of extern "C" */
#endif /* __cplusplus */

#endif //_MEM_ALLOC_HOOK_H_
