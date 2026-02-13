cmake_minimum_required(VERSION 3.10)

project(unit_test)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/blob_core.c
    ${CMAKE_SOURCE_DIR}/src/blob_node.c
    ${CMAKE_SOURCE_DIR}/src/blob_node_tree.c
    ${CMAKE_SOURCE_DIR}/src/blob.c
    ${CMAKE_SOURCE_DIR}/test/unit_test.c
)

# Executable
add_executable(blob_unit_test ${SOURCE_FILES})
target_compile_definitions(blob_unit_test PRIVATE BLOB_FILE=1 _CRT_SECURE_NO_WARNINGS)
set_target_properties(blob_unit_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
