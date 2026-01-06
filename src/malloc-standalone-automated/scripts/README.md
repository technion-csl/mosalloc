# `scripts/`

This directory contains the automation helper that turns a **glibc checkout** into a buildable, **LD_PRELOAD-friendly** standalone `malloc` shared library by repeatedly building and fixing *missing header* dependencies.

The main entry is: **`autoinclude.py`**.

---

## What `autoinclude.py` does

### The core idea

We want to compile glibc’s allocator sources (e.g. `malloc.c`) **outside of glibc**, so compilation fails because glibc normally relies on a huge internal include graph, loader-only headers, and build-system-provided macros.

`autoinclude.py` automates the mechanical part of this process:

1. **Try to build** the shared library target (`make malloc-standalone`).
2. If compilation fails due to missing headers:

   * Decide whether the header should be:

     * **mirrored** from glibc into our repo (`include/glibc-src/...`), or
     * **provided manually** via a shim (`include/special_shims/...`)
3. Repeat until the build succeeds or we stop making progress.

The script is intentionally conservative: it fixes **missing header includes**, but it does *not* try to invent complicated macro definitions anymore.

---

## How each iteration compiles

Each iteration runs the Makefile rule:

* **`make -s malloc-standalone`**

The script computes a correct `CPPFLAGS` for the build and injects it into the environment:

* `CPPFLAGS` contains:

  * forced includes:

    * `-include include/config.h`
    * `-include include/auto_stubs.h`
    * `-include include/glibc_compat.h`
  * plus **`-I...` for every directory under `include/`**, so anything mirrored under `include/glibc-src/` becomes visible automatically.

When the build eventually succeeds, the final computed `CPPFLAGS` is written to:

* `build/CPPFLAGS`

so future `make` runs can reuse a stable include setup.

> Exact GCC flags / options are documented in `automated/README.md` (and in the Makefile comments). This README focuses on the automation algorithm and flow.

---

## Function-level breakdown

Below is the script logic at the “what each function does” resolution.

### Setup / preconditions

* **`ensure_dirs()`**

  * Creates required directories (`build/`, `include/`, `include/glibc-src/`, `include/special_shims/`, `src/`).
  * Ensures `include/auto_stubs.h` exists **once** (it’s then treated as repo-owned/manual).

* **`require_repo_owned_headers()`**

  * Ensures `include/glibc_compat.h` exists.
  * The script **does not generate** it (avoids “two sources of truth”).

* **`ensure_glibc_sources()`**

  * Copies selected glibc `.c` sources into `src/` as “imports”, so the repo stays small.
  * Typical imports include:

    * `malloc/malloc.c`
    * `malloc/arena.c`
    * `malloc/morecore.c`
    * `sysdeps/generic/malloc-hugepages.c` (with fallback)
  * This means you don’t commit these large upstream files; they are refreshed from your glibc checkout.

---

### The include path model

* **`collect_include_dirs()`**

  * Recursively lists *every directory* under `include/`.
  * Orders them “outer to inner” so broad include directories are searched first.

* **`compute_cppflags()`**

  * Builds the final `CPPFLAGS` string:

    * forces important headers with `-include ...`
    * adds `-I...` for all include directories from `collect_include_dirs()`

* **`write_cppflags_file()`**

  * Writes the successful `CPPFLAGS` to `build/CPPFLAGS` so the include setup can be reused.

---

### Error parsing (header-only)

* **`parse_missing_headers(err_text)`**

  * Scans compilation output for patterns like:

    * `fatal error: X: No such file or directory`
  * Returns a de-duplicated list of missing header names.

---

### Deciding: mirror vs shim

Some headers are considered “too internal” and must be handled manually:

* Examples: `ldsodefs.h`, `libc-internal.h`, `shlib-compat.h`, `random-bits.h`,
  `libc-lock.h`, `setvmaname.h`, `elf/dl-tunables.h`.

That policy is represented by:

* `BLOCKED_HEADERS` (by basename)
* `SPECIAL_SHIMS` (explicit shim paths, including subdirs like `elf/...`)

And enforced via:

* **`require_special_shim(hdr)`**

  * For blocked headers, checks that the shim exists under:

    * `include/special_shims/<basename>` (most cases), or
    * `include/special_shims/<subdir>/<file>` (e.g. `elf/dl-tunables.h`)
  * If missing → the script stops and tells you exactly which file is required.

---

### Mirroring headers from glibc

If a missing header is *not blocked*, the script mirrors it from glibc:

* **`find_candidates_in_glibc(src_path, hdr)`**

  * Uses `find ... -name <basename>` to locate possible matches.
  * Returns a list of candidate paths.

* **`choose_best(cands)`**

  * Picks a “best” candidate using heuristics:

    * prefer `sysdeps/generic/`
    * then `sysdeps/<ARCH>/`
    * then `include/`
    * otherwise first match
  * Also tries to avoid irrelevant variants (like x32) when possible.

* **`mirror_preserving_tree(src_abs)`**

  * Copies the chosen glibc header into `include/glibc-src/...`
  * Preserves the path relative to the glibc root.
  * Example:

    * glibc: `.../glibc/stdlib/stdbit.h`
    * repo:  `include/glibc-src/stdlib/stdbit.h`

* **`mirror_flat_alias(hdr, src_abs)`**

  * If the include was a “bare” include like `<foo.h>`, optionally creates:

    * `include/glibc-src/include/foo.h`
  * This helps satisfy includes that expect the “flat” glibc include layout.

---

### Hygiene / de-duplication

* **`dedupe_mirror()`**

  * For a small whitelist of headers that tend to appear in multiple locations,
    deletes duplicates and keeps a single canonical copy.
  * This avoids accidental inclusion of a wrong duplicate.

* **`snapshot_mirror_files()`**

  * Used for logging: captures “before” and “after” states of mirrored files,
    so the script can print what it added in the current iteration.

---

## Main loop summary (how it all ties together)

Inside `main()`:

1. Setup directories and verify repo-owned prerequisites exist.
2. Copy required glibc sources into `src/`.
3. Loop up to `MAX_ITERS` times:

   * compute `CPPFLAGS`
   * run: `make -s malloc-standalone`
   * if success:

     * write `build/CPPFLAGS`
     * exit
   * else:

     * parse missing headers
     * for each missing header:

       * if blocked → require shim in `include/special_shims/`
       * else → mirror header into `include/glibc-src/`
     * retry

The loop stops early if:

* the build fails but no missing-header errors are detected, or
* nothing new was added in an iteration, or
* a required manual shim is missing.

---

## Usage

From `automated/`:

```bash
# Use an existing glibc checkout:
make run_build GLIBC_SRC=/path/to/glibc

# Or let Makefile clone glibc and then build:
make
```

Or run directly:

```bash
MAX_ITERS=100 python3 scripts/autoinclude.py --glibc-src /path/to/glibc
```

---

## Notes (where to fix things)

* If the build fails with a blocked header:

  * add the shim under `include/special_shims/...` (preferred)
* If the build fails with missing macros / symbols:

  * fix explicitly in `include/auto_stubs.h` or (preferably) in a shim header.
