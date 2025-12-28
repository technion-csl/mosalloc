#pragma once

/* Minimal atomic shim for the allocator.
   Backed by GCC/Clang __atomic builtins only. */

#if !defined(__GNUC__) && !defined(__clang__)
# error "This atomic shim requires GCC/Clang __atomic builtins"
#endif

/* ---- Fences ---- */
#define atomic_full_barrier()        __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define atomic_read_barrier()        __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define atomic_write_barrier()       __atomic_thread_fence(__ATOMIC_RELEASE)

/* ---- Loads / Stores ---- */
#define atomic_load_relaxed(mem) \
  __atomic_load_n((mem), __ATOMIC_RELAXED)

#define atomic_store_relaxed(mem, val) \
  __atomic_store_n((mem), (val), __ATOMIC_RELAXED)

/* ---- Exchange ---- */
#define atomic_exchange_relaxed(mem, val) \
  __atomic_exchange_n((mem), (val), __ATOMIC_RELAXED)

#define atomic_exchange_acquire(mem, val) \
  __atomic_exchange_n((mem), (val), __ATOMIC_ACQUIRE)

#define atomic_exchange_release(mem, val) \
  __atomic_exchange_n((mem), (val), __ATOMIC_RELEASE)

/* ---- Compare-Exchange returning OLD value (glibc style) ----
   Returns the previous value observed at *mem (whether or not swap happened). */
#define atomic_compare_and_exchange_val_acq(mem, newval, oldval)      \
  ({ __auto_type __old = __atomic_load_n((mem), __ATOMIC_RELAXED);     \
     __atomic_compare_exchange_n((mem), &__old, (newval),              \
                                 /*weak=*/0, __ATOMIC_ACQUIRE,         \
                                 __ATOMIC_RELAXED);                     \
     __old; })

#define atomic_compare_and_exchange_val_rel(mem, newval, oldval)      \
  ({ __auto_type __old = __atomic_load_n((mem), __ATOMIC_RELAXED);     \
     __atomic_compare_exchange_n((mem), &__old, (newval),              \
                                 /*weak=*/0, __ATOMIC_RELEASE,         \
                                 __ATOMIC_RELAXED);                     \
     __old; })

/* ---- Compare-Exchange returning bool (glibc semantics) ----
   IMPORTANT: legacy glibc macro returns nonzero on *failure*.
   i.e., returns 0 iff the exchange succeeded. */
#define atomic_compare_and_exchange_bool_acq(mem, newval, expected)    \
  ({ __auto_type __exp = (expected);                                   \
     bool __ok = __atomic_compare_exchange_n((mem), &__exp,            \
                      (newval), /*weak=*/0, __ATOMIC_ACQUIRE,          \
                      __ATOMIC_RELAXED);                                \
     !__ok; /* true on failure, matching glibc */                      \
  })

/* ---- C11-style helpers used by your code (return true on success) ---- */
#define atomic_compare_exchange_relaxed(mem, expected_ptr, desired) \
  __atomic_compare_exchange_n((mem), (expected_ptr), (desired),     \
                              /*weak=*/0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define atomic_compare_exchange_acquire(mem, expected_ptr, desired) \
  __atomic_compare_exchange_n((mem), (expected_ptr), (desired),     \
                              /*weak=*/0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

#define atomic_compare_exchange_release(mem, expected_ptr, desired) \
  __atomic_compare_exchange_n((mem), (expected_ptr), (desired),     \
                              /*weak=*/0, __ATOMIC_RELEASE, __ATOMIC_RELAXED)

/* Weak variants (may spuriously fail): */
#define atomic_compare_exchange_weak_relaxed(mem, expected_ptr, desired) \
  __atomic_compare_exchange_n((mem), (expected_ptr), (desired),          \
                              /*weak=*/1, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define atomic_compare_exchange_weak_acquire(mem, expected_ptr, desired) \
  __atomic_compare_exchange_n((mem), (expected_ptr), (desired),          \
                              /*weak=*/1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

#define atomic_compare_exchange_weak_release(mem, expected_ptr, desired) \
  __atomic_compare_exchange_n((mem), (expected_ptr), (desired),          \
                              /*weak=*/1, __ATOMIC_RELEASE, __ATOMIC_RELAXED)

/* ---- Fetch ops used by your code ---- */
#define atomic_fetch_add_relaxed(mem, operand) \
  __atomic_fetch_add((mem), (operand), __ATOMIC_RELAXED)

/* (Add acquire/release forms only if you really need them later) */

/* ---- Utilities ---- */
#define atomic_spin_nop() do { /* nothing */ } while (0)

/* Max(*mem, value) with relaxed ordering (matches prior usage). */
#define atomic_max(mem, value)                                          \
  do {                                                                  \
    __auto_type __val = (value);                                        \
    __auto_type __old = __atomic_load_n((mem), __ATOMIC_RELAXED);       \
    while (__old < __val &&                                             \
           !__atomic_compare_exchange_n((mem), &__old, __val,           \
                                        /*weak=*/0, __ATOMIC_RELAXED,   \
                                        __ATOMIC_RELAXED)) {            \
      /* __old reloaded by compare_exchange on failure */               \
    }                                                                   \
  } while (0)
