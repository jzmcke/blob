cmake_minimum_required(VERSION 3.10)

project(network_test)

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
    ${CMAKE_SOURCE_DIR}/src/blob_udp.c
    ${CMAKE_SOURCE_DIR}/src/blob_frag_tx.c
    ${CMAKE_SOURCE_DIR}/src/blob_jbuf_frag.c
    ${CMAKE_SOURCE_DIR}/src/packet.c
    ${CMAKE_SOURCE_DIR}/test/network_test.c
)

# Executable
add_executable(blob_network_test ${SOURCE_FILES})
target_compile_definitions(blob_network_test PRIVATE BLOB_FILE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(blob_network_test ws2_32) # Link Winsock
set_target_properties(blob_network_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# Triangle Test Executable
set(TRIANGLE_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/blob_core.c
    ${CMAKE_SOURCE_DIR}/src/blob_node.c
    ${CMAKE_SOURCE_DIR}/src/blob_node_tree.c
    ${CMAKE_SOURCE_DIR}/src/blob.c
    ${CMAKE_SOURCE_DIR}/src/blob_udp.c
    ${CMAKE_SOURCE_DIR}/src/blob_frag_tx.c
    ${CMAKE_SOURCE_DIR}/src/blob_jbuf_frag.c
    ${CMAKE_SOURCE_DIR}/src/packet.c
    ${CMAKE_SOURCE_DIR}/test/triangle_test.c
)
add_executable(blob_triangle_test ${TRIANGLE_SOURCE_FILES})
target_compile_definitions(blob_triangle_test PRIVATE BLOB_FILE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(blob_triangle_test ws2_32)
set_target_properties(blob_triangle_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Loopback Test Executable
set(LOOPBACK_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/blob_core.c
    ${CMAKE_SOURCE_DIR}/src/blob_node.c
    ${CMAKE_SOURCE_DIR}/src/blob_node_tree.c
    ${CMAKE_SOURCE_DIR}/src/blob.c
    ${CMAKE_SOURCE_DIR}/src/blob_udp.c
    ${CMAKE_SOURCE_DIR}/src/blob_frag_tx.c
    ${CMAKE_SOURCE_DIR}/src/blob_jbuf_frag.c
    ${CMAKE_SOURCE_DIR}/src/packet.c
    ${CMAKE_SOURCE_DIR}/src/blob_ws_win.c
    ${CMAKE_SOURCE_DIR}/test/blob_loopback_test.c
)
add_executable(blob_loopback_test ${LOOPBACK_SOURCE_FILES})
target_compile_definitions(blob_loopback_test PRIVATE BLOB_FILE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(blob_loopback_test ws2_32)
set_target_properties(blob_loopback_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
