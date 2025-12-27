#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define ROUND_UP(num, mul) \
    ((((size_t)(num) + (size_t)(mul) - 1) / (size_t)(mul)) * (size_t)(mul))
#define ROUND_DOWN(num, mul) \
    ((((size_t)(num)) / ((size_t)(mul))) * ((size_t)(mul)))
#define IS_ALIGNED(num, alignment) (((off64_t)(num) % (size_t)(alignment)) == 0)

#define __WRITE_ERROR_AND_EXIT(msg)    \
{\
    ssize_t res = 0; \
    res |= write(STDERR_FILENO, __FILE__, strlen(__FILE__)); \
    res |= write(STDERR_FILENO, ":", 1); \
    res |= write(STDERR_FILENO, __FUNCTION__, strlen(__FUNCTION__)); \
    res |= write(STDERR_FILENO, "\n", 1); \
    char msg_buf[] = msg; \
    res |= write(STDERR_FILENO, msg_buf, sizeof(msg_buf)); \
    res |= write(STDERR_FILENO, "\n", 1); \
    if (res) sync(); \
    _exit(1); \
}

/* use write and sync for throwing exceptions because they are not allocating
 * new memory or calling memory allocations APIs like other printing APIs or
 * c++ std exceptions.
 * Using or calling memory allocation APIs during thrwoing exceptions causes
 * infinite recursive calls to malloc (because these excpetions will be
 * thrown from malloc, or other allocation APIs)
*/
#define THROW_EXCEPTION(msg)    \
{\
    char prefix_buf[] = "Exception thrown at: "; \
    ssize_t res = write(STDERR_FILENO, prefix_buf, sizeof(prefix_buf)); \
    if (res) sync(); \
    __WRITE_ERROR_AND_EXIT(msg) \
}

#define ASSERT(cond) \
{\
    if (!(cond)) { \
        __WRITE_ERROR_AND_EXIT("Code assertion!") \
}}

/**********************************************
* mmap special flags for using huge pages
**********************************************/
#define MMAP_PROTECTION (PROT_READ | PROT_WRITE)
#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)

enum class PageSize : size_t {
    BASE_4KB = 4096, // (1 << 12)
    HUGE_2MB = 2097152, // (1 << 21)
    HUGE_1GB = 1073741824, // (1 << 30)
    UNKNOWN = 0
};

#endif //_GLOBALS_H_
