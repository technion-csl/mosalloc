list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(SelectMosallocImpl)

file(GLOB SRCS "*.cc")
file(GLOB HDRS "${CMAKE_SOURCE_DIR}/include/*.h")

# The following workaround is to prevent building the test apps with hooks.cc.
# If the test apps are linked with hooks.cc, then the constructor is called at runtime,
# which overrides the memory functions, reads the environment variables, etc.
# The test apps don't need the constructor, they only need the API of the mosalloc classes.
#
# Build this API library in both legacy and automated modes.
set(API_SRCS ${SRCS})
list(FILTER API_SRCS EXCLUDE REGEX ".*/hooks.cc")

add_library(${API_LIBRARY} SHARED ${API_SRCS} ${HDRS})
target_link_libraries(${API_LIBRARY} dl)
target_include_directories(${API_LIBRARY} PUBLIC ../include)
message(STATUS "api-library: ${API_LIBRARY}")


select_mosalloc_impl(MOSALLOC_IMPL)
message(STATUS "Selected Mosalloc implementation: ${MOSALLOC_IMPL}")

if(MOSALLOC_IMPL STREQUAL "automated")
    message(STATUS "Building Mosalloc with integrated automated malloc backend")

    # We do not want Mosalloc to depend on a second standalone malloc DSO.
    # Instead, embed standmalloc's object library into the final libmosalloc.so.
    set(STANDMALLOC_BUILD_SHARED OFF CACHE BOOL "" FORCE)

    add_subdirectory(malloc-standalone-automated EXCLUDE_FROM_ALL)

    if(NOT TARGET standmalloc_obj)
        message(FATAL_ERROR
            "malloc-standalone-automated did not define standmalloc_obj. "
            "Please check the standalone submodule CMakeLists.txt.")
    endif()

    add_library(${PROJECT_NAME} SHARED
        ${SRCS}
        ${HDRS}
        $<TARGET_OBJECTS:standmalloc_obj>
    )

    add_dependencies(${PROJECT_NAME} standmalloc_obj)

    target_include_directories(${PROJECT_NAME} PRIVATE ../include)

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        MOSALLOC_AUTOMATED_BACKEND
    )

    target_link_libraries(${PROJECT_NAME} dl pthread)

else()
    message(STATUS "Building Mosalloc with legacy __morecore backend")

    add_library(${PROJECT_NAME} SHARED ${SRCS} ${HDRS})
    target_link_libraries(${PROJECT_NAME} dl)
    target_include_directories(${PROJECT_NAME} PRIVATE ../include)
endif()