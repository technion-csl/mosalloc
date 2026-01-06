## `automated/src/` — Allocator Sources & Entry Points

This directory contains the **runtime source files** that make up the standalone allocator shared object (`libmalloc_try.so`) in the automated build.

Some files here are **hand-written glue code**, while others are **copied verbatim from glibc** by the automation script.

---

### Files in this directory

#### `malloc.c` *(auto-copied)*

* **Origin:** `glibc/malloc/malloc.c`
* **Role:**
  The core glibc allocator implementation (ptmalloc).
* **How it gets here:**
  Automatically copied from the glibc source tree by `scripts/autoinclude.py`.
* **Notes:**

  * Included verbatim (1:1 copy).
  * May `#include` other glibc `.c` files (e.g. `arena.c`, `morecore.c`).
  * Built with hidden visibility to avoid leaking glibc internals.

---

#### `arena.c` *(auto-copied)*

* **Origin:** `glibc/malloc/arena.c`
* **Role:**
  Implements arena management for multi-threaded allocation.
* **How it gets here:**
  Copied automatically from glibc.
* **Notes:**

  * Not built as a separate compilation unit; included from `malloc.c`.

---

#### `malloc-hugepages.c` *(auto-copied)*

* **Origin:** `glibc/sysdeps/generic/malloc-hugepages.c`
* **Role:**
  Handles transparent huge page (THP) related logic used by the allocator.
* **How it gets here:**
  Copied automatically when required to resolve link-time references.
* **Notes:**

  * Included only because modern glibc malloc expects it.

---

#### `exports.c`

* **Role:**
  Defines the **public symbols** exported by the standalone allocator:

  * `malloc`
  * `free`
  * `calloc`
  * `realloc`
  * `aligned_alloc`
  * (optionally) `memalign`
* **Purpose:**
  Acts as the **LD_PRELOAD interposition layer**, forwarding calls into
  glibc-internal entry points like `__libc_malloc`.
* **Notes:**

  * Carefully avoids exporting unnecessary POSIX/GNU APIs.
  * Central place for tracing, symbol hygiene, and ABI control.

---

#### `errno_shim.c`

* **Role:**
  Provides a standalone definition of `__set_errno`.
* **Why it exists:**
  glibc malloc code uses `__set_errno()` internally, which is normally
  provided by libc internals and not available in a standalone build.
* **Notes:**

  * Minimal implementation.
  * Required to satisfy link-time references.

---

#### `hooks.c`

* **Role:**
  Optional hook points and debugging helpers.
* **Purpose:**
  Reserved for allocator instrumentation, tracing hooks, or experiments
  (e.g. intercepting allocator lifecycle events).
* **Notes:**

  * Not required for correctness.
  * Safe place to add experimental logic without touching glibc code.

---

### Important design notes

* **This directory is not fully static**
  Some files (`malloc.c`, `arena.c`, `malloc-hugepages.c`, `morecore.c`) are
  *generated/copied* by the automation script and should not be edited directly.

* **Manual vs generated code**

  * Manual: `exports.c`, `errno_shim.c`, `hooks.c`
  * Generated: glibc allocator sources

* **Single responsibility**

  * `src/` contains *code only*.
  * Compatibility logic lives in `include/`.
  * Automation logic lives in `scripts/`.