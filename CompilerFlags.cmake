# Enable compiler flags only if supported by the current compiler. Taken from:
# https://stackoverflow.com/questions/33222569/best-way-to-portably-set-compiler-options-like-wall-and-pedantic-in-cmake

include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

# example usage
enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")
enable_cxx_compiler_flag_if_supported("-pedantic-errors")
enable_cxx_compiler_flag_if_supported("-O3")
#enable_cxx_compiler_flag_if_supported("-Wconversion")
#enable_cxx_compiler_flag_if_supported("-Wsign-conversion")

