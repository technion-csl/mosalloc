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

# copy the reserveHugePages.sh script to binary directory to be used in the
# tests which require reserving huge pages
configure_file("${CMAKE_SOURCE_DIR}/reserveHugePages.sh" ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
