file(GLOB TEST_SRCS "*.cc")
file(GLOB TEST_HDRS "*.h")

# ============== gtest dependency - start ==============
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
# ============== gtest dependency - end ==============

set(TEST_NAME ${PROJECT_NAME}-test)
add_executable(${TEST_NAME} ${TEST_SRCS} ${TEST_HDRS})
target_link_libraries(${TEST_NAME} ${GTEST_LIBRARIES} pthread gtest_main ${API_LIBRARY})
add_test(NAME ${TEST_NAME}-run COMMAND ${TEST_NAME})

# Test with libmalloc_min_api.so by linking directly (API library without hooks.cc)
set(TEST_NAME_MALLOC_MIN ${PROJECT_NAME}-test-malloc-min)
add_executable(${TEST_NAME_MALLOC_MIN} ${TEST_SRCS} ${TEST_HDRS})
target_link_libraries(${TEST_NAME_MALLOC_MIN} ${GTEST_LIBRARIES} pthread gtest_main malloc_min_api)
add_test(NAME ${TEST_NAME_MALLOC_MIN}-run COMMAND ${TEST_NAME_MALLOC_MIN})

# Test with libmalloc_auto_api.so by linking directly (API library without hooks.cc)
set(TEST_NAME_MALLOC_AUTO ${PROJECT_NAME}-test-malloc-auto)
add_executable(${TEST_NAME_MALLOC_AUTO} ${TEST_SRCS} ${TEST_HDRS})
target_link_libraries(${TEST_NAME_MALLOC_AUTO} ${GTEST_LIBRARIES} pthread gtest_main malloc_auto_api)
add_test(NAME ${TEST_NAME_MALLOC_AUTO}-run COMMAND ${TEST_NAME_MALLOC_AUTO})

# copy the reserveHugePages.sh script to binary directory to be used in the
# tests which require reserving huge pages
configure_file("${CMAKE_SOURCE_DIR}/reserveHugePages.sh" ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
