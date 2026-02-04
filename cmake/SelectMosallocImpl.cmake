include(CheckCSourceCompiles)

function(select_mosalloc_impl out_var)
  option(MOSALLOC_FORCE_AUTOMATED "Force using src/malloc-standalone-automated" OFF)

  if(MOSALLOC_FORCE_AUTOMATED)
    set(${out_var} "automated" PARENT_SCOPE)
    return()
  endif()

  check_c_source_compiles([=[
    #include <features.h>
    #if !defined(__GLIBC__)
    # error "not glibc"
    #endif
    #if (__GLIBC__ < 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 32)
    # error "glibc too old"
    #endif
    int main(void) { return 0; }
  ]=] GLIBC_AT_LEAST_2_32)

  if(GLIBC_AT_LEAST_2_32)
    set(${out_var} "automated" PARENT_SCOPE)
  else()
    set(${out_var} "legacy" PARENT_SCOPE)
  endif()
endfunction()
