# `minimal/src/` — Allocator Sources (Minimal Build)

This directory contains the **actual compiled sources** for the minimal standalone allocator.
Unlike the automated build (which imports allocator sources during the build), the minimal build keeps the core `.c` files directly in the repo.

---

## Files

### `malloc.c`

The main allocator implementation imported from glibc **with small edits** to keep the standalone build minimal and reduce dependencies.

Typical edits here are *not algorithmic changes*, but “plumbing” changes such as:

* reducing reliance on glibc-only infrastructure
* trimming optional features that would pull extra headers/files
* keeping includes and feature gates compatible with the local `include/` layout

This file is the center of everything: it includes/uses arena code, internal macros, and (in this repo) expects the `morecore` setup provided by `morecore.c`.

---

### `arena.c`

glibc’s arena / heap management logic.

This is responsible for:

* per-arena bookkeeping (main arena + additional arenas when needed)
* handling heap growth, trimming, and coordination with `malloc.c`
* managing locking patterns used by multi-threaded allocation paths

Even in a “minimal” build, keeping `arena.c` is important for correctness because glibc malloc is designed around arenas.

---

### `morecore.c`

**Custom standalone morecore implementation** (not the stock glibc `morecore.c`).

Key differences in this repo:

* In glibc, the allocator usually calls an internal `__glibc_morecore` / `__sbrk` path and wiring is handled via glibc build logic.
* In this minimal build, the wiring is explicit:

  * `malloc.c` contains the typedef / declaration expectations.
  * `morecore.c` provides the implementation and the **`morecore` macro mapping** used by the allocator.

This is also where the project can expose a public `morecore` symbol (when desired) for experiments/tests (e.g., the smoke test calling `dlsym("morecore")`).

---

### `exports.c`

The **LD_PRELOAD interposition layer**: it exports the public malloc API that programs call:

* `malloc`
* `free`
* `calloc`
* `realloc`
* (optionally) `aligned_alloc`, `memalign`, etc.

These wrappers call into glibc-internal entry points from `malloc.c` (e.g. `__libc_malloc`, `__libc_free`, …) and optionally emit trace logs via `include/trace.h`.

This file is intentionally small and explicit: it’s the boundary between **the system program ABI** and **glibc allocator internals**.

---

## Notes / Design Intent

* The goal of `minimal/src/` is a **stable, understandable baseline**:

  * allocator code is mostly “as glibc wrote it”
  * any portability glue is either in `include/` shims or small local edits in `malloc.c`
  * the `morecore` mechanism is explicit and experiment-friendly
