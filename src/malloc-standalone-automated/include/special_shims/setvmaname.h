#ifndef MALLOC_STANDALONE_SETVMANAME_H
#define MALLOC_STANDALONE_SETVMANAME_H

#include <stddef.h>

/*
 * glibc uses this to label VMAs via prctl(PR_SET_VMA...).
 * Standalone malloc does not need it; keep as no-op.
 */
static inline void
__set_vma_name (void *start __attribute__((unused)),
                size_t len __attribute__((unused)),
                const char *name __attribute__((unused)))
{
  /* no-op */
}

#endif /* MALLOC_STANDALONE_SETVMANAME_H */
