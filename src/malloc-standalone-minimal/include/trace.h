#pragma once
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* ---------- tiny, allocation-free tracing ---------- */
static inline int trace_enabled(void) {
    static int cached = -1;
    if (cached == -1) {
        const char *v = getenv("MALLOC_FORK_TRACE");  // does not allocate in glibc
        cached = (v && v[0] == '1') ? 1 : 0;
    }
    return cached;
}
static inline void trace_msg(const char *s) {
    if (!trace_enabled()) return;
    (void)!write(2, s, (unsigned)strlen(s));
    (void)!write(2, "\n", 1);
}
