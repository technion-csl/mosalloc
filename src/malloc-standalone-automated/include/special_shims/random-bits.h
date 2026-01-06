#ifndef MALLOC_FORK_RANDOM_BITS_H_SHIM
#define MALLOC_FORK_RANDOM_BITS_H_SHIM

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* Very simple "good enough" random bits for malloc tuning.
   Signature matches glibc's helper: take a seed pointer and bit count.  */
static inline uint64_t
random_bits (unsigned int *seed, int bits)
{
  uint64_t r;

  if (seed && *seed)
    r = *seed;
  else
    r = (uint64_t) time (NULL);

  /* LCG step */
  r = r * 1103515245u + 12345u;

  if (seed)
    *seed = (unsigned int) r;

  if (bits >= 64)
    return r;
  if (bits <= 0)
    return 0;

  return r & ((1ULL << bits) - 1);
}

#endif /* MALLOC_FORK_RANDOM_BITS_H_SHIM */
