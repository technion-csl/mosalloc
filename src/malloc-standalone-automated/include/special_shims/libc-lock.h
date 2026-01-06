#ifndef MALLOC_STANDALONE_LIBC_LOCK_H
#define MALLOC_STANDALONE_LIBC_LOCK_H

#include <pthread.h>

/* Minimal lock type for standalone malloc. */
typedef pthread_mutex_t __libc_lock_t;

#define __libc_lock_define(CLASS, NAME) \
  CLASS __libc_lock_t NAME

#define __libc_lock_define_initialized(CLASS, NAME) \
  CLASS __libc_lock_t NAME = PTHREAD_MUTEX_INITIALIZER

#define LLL_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#ifndef _LIBC_LOCK_INITIALIZER
# define _LIBC_LOCK_INITIALIZER LLL_LOCK_INITIALIZER
#endif

#define __libc_lock_init(NAME)    (pthread_mutex_init (&(NAME), NULL))
#define __libc_lock_lock(NAME)    (pthread_mutex_lock (&(NAME)))
#define __libc_lock_unlock(NAME)  (pthread_mutex_unlock (&(NAME)))
#define __libc_lock_destroy(NAME) (pthread_mutex_destroy (&(NAME)))

#endif /* MALLOC_STANDALONE_LIBC_LOCK_H */
