# === Compiler ===
CC := gcc

# === Config for the Python autoinclude script ===
GLIBC_SRC  ?= glibc
MAX_ITERS  ?= 100
GLIBC_REPO ?= https://sourceware.org/git/glibc.git

# === Include paths / compat are handled by CPPFLAGS ===
# The autoinclude script computes and passes CPPFLAGS via env during its loop,
# and writes the last-good flags to build/CPPFLAGS for later plain `make` runs.
CPPFLAGS ?= $(shell cat build/CPPFLAGS 2>/dev/null)

# === Debug/release toggle ===
# DEBUG=1  -> -O0 -g3 (best for gdb + macro-heavy glibc code)
# DEBUG=0  -> -O2 -g  (faster, still has symbols)
DEBUG ?= 0
ifeq ($(DEBUG),1)
  CFLAGS_BASE := -fPIC -O0 -g3 -pthread -std=gnu11 -Wall -Wextra
else
  CFLAGS_BASE := -fPIC -O2 -g  -pthread -std=gnu11 -Wall -Wextra
endif

CFLAGS := $(CFLAGS_BASE)
CFLAGS += -Wimplicit-function-declaration

# === Optional: depfiles for incremental rebuilds (enabled by default) ===
#
# Why this exists even though the "main" workflow is script-driven:
# - The script drives the first successful build, but after that you often
#   want quick manual iterations (edit a shim/header -> rebuild) without
#   running the whole loop again.
# - Depfiles make `make malloc-standalone` correctly rebuild when headers change.
#
# -MMD:
#   Generate a .d file next to each .o describing header dependencies,
#   but *exclude* system headers (so deps remain portable and smaller).
#
# -MP:
#   Add a phony target for each header listed in the .d file. This prevents:
#     make: *** No rule to make target 'include/special_shims/foo.h', needed by build/malloc.o.  Stop.
#   when a header is removed/renamed (stale depfiles). With -MP, missing headers
#   become empty phony targets, so you can keep building without having to
#   `make clean` immediately.
#
# Example of what -MP prevents:
#   1) build/malloc.o was built when it included include/special_shims/ldsodefs.h
#   2) you later rename/delete that shim file
#   3) without -MP, the stale build/malloc.d still mentions it -> make aborts
#   4) with -MP, make doesn’t abort; it just knows it may need to rebuild.
USE_DEPS ?= 1
ifeq ($(USE_DEPS),1)
  CFLAGS += -MMD -MP
endif

# === Link flags ===
LDFLAGS := -shared -pthread -Wl,-z,defs
LIBS    := -ldl

# === Sources / Targets ===
SRC    := src/malloc.c src/exports.c src/malloc-hugepages.c src/errno_shim.c
OBJ    := $(SRC:src/%.c=build/%.o)

ifeq ($(USE_DEPS),1)
  DEPS := $(OBJ:.o=.d)
endif

TARGET := build/libmalloc_auto.so

# Hide glibc internals by default so we export only the intended API surface.
# (Your public API is exported via exports.c and explicit symbol choices.)
build/malloc.o: CFLAGS += -fvisibility=hidden

# ============================================================
# Main rules
# ============================================================

# Default: ensure glibc exists, then run autoinclude build loop
all: clone-glibc run_build

# Target for CMake to generate source files (deletes glibc after success to save space)
mosalloc: clone-glibc run_build patch-morecore
	@echo "Cleaning up glibc source to save space..."
	rm -rf $(GLIBC_SRC)

# Patch malloc.c to use customizable __morecore function pointer
patch-morecore:
	@echo "Patching malloc.c to use __morecore function pointer..."
	sed -i 's|#define MORECORE         (\*__glibc_morecore)|#define MORECORE         (*__morecore)\nvoid * __glibc_morecore (ptrdiff_t);\nvoid *(*__morecore)(ptrdiff_t) = __glibc_morecore;|' src/malloc.c

# This is the exact target the autoinclude script builds each iteration
malloc-standalone: $(TARGET)

run_build:
	MAX_ITERS=$(MAX_ITERS) python3 scripts/autoinclude.py --glibc-src $(GLIBC_SRC)

clone-glibc:
	@if [ -d "$(GLIBC_SRC)" ]; then \
	    echo "glibc source already exists at $(GLIBC_SRC); doing nothing."; \
	else \
	    echo "Cloning glibc from $(GLIBC_REPO) into $(GLIBC_SRC)"; \
	    git clone $(GLIBC_REPO) $(GLIBC_SRC); \
	fi

print-preload:
	@echo $(abspath $(TARGET))

# ============================================================
# Build rules
# ============================================================

build:
	mkdir -p build

build/%.o: src/%.c | build
	@echo "== CC CPPFLAGS = $(CPPFLAGS)"
	@echo "== CC CFLAGS   = $(CFLAGS)"
	@err=build/$*.err; \
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ 2> $$err; rc=$$?; \
	if [ $$rc -ne 0 ]; then \
	  cp $$err build/compile.err; \
	  echo; echo "== compile errors in $$err =="; \
	  cat $$err; \
	  exit $$rc; \
	else \
	  rm -f $$err; \
	  echo "CC  $< -> $@"; \
	fi

$(TARGET): $(OBJ) | build
	@err=build/link.err; \
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS) 2> $$err; rc=$$?; \
	if [ $$rc -ne 0 ]; then \
	  echo; echo "== link errors in $$err =="; \
	  cat $$err; \
	  exit $$rc; \
	else \
	  rm -f $$err; \
	  echo "LD  $@"; \
	fi

# Optional helper: write a flattened dependency list for inspection
deps: | build
	$(CC) -M $(CPPFLAGS) $(filter-out -MMD -MP,$(CFLAGS)) $(SRC) > build/malloc.mm
	cat build/malloc.mm \
	| sed -e ':a' -e '/\\$$/N; s/\\\n/ /; ta' -e 's/^[^:]*: *//' \
	| tr ' ' '\n' | sed '/^$$/d' | sort -u > build/malloc.deps.txt
	@echo "Wrote build/malloc.deps.txt"

check-exports: $(TARGET)
	nm -D $(TARGET) | egrep ' (T|W) (malloc|free|calloc|realloc|posix_memalign|aligned_alloc|morecore)$$' \
	  || (echo "Missing allocator symbols"; exit 1)

clean:
	rm -rf build include/glibc-src include/compat_next
	rm -f src/arena.c src/malloc.c src/malloc-hugepages.c src/morecore.c
	rm -rf $(GLIBC_SRC)
.PHONY: all mosalloc malloc-standalone run_build clone-glibc clean deps check-exports print-preload

ifeq ($(USE_DEPS),1)
-include $(DEPS)
endif
