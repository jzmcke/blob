cmake_minimum_required(VERSION 3.10)

project(blob_core_lib)

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
    blob_core.def
)

# Shared Library
add_library(blob_core SHARED ${SOURCE_FILES})
target_compile_definitions(blob_core PRIVATE BLOB_WINDOWS_DLL _CRT_SECURE_NO_WARNINGS)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

set_target_properties(blob_core PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

if(WIN32)
    target_link_libraries(blob_core PRIVATE ws2_32)
endif()
