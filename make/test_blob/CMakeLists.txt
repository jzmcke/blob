cmake_minimum_required(VERSION 3.10)

project(blob)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# OpenSSL may have different paths on different systems
find_package(OpenSSL REQUIRED)

# Replace websockets with the correct package name if different
find_package(libwebsockets REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

# Source files
set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/blob_core.c
    ${CMAKE_SOURCE_DIR}/src/blob_node.c
    ${CMAKE_SOURCE_DIR}/src/blob_node_tree.c
    ${CMAKE_SOURCE_DIR}/src/blob_minws.c
    ${CMAKE_SOURCE_DIR}/src/blob_espws.c
    ${CMAKE_SOURCE_DIR}/src/blob.c
    ${CMAKE_SOURCE_DIR}/src/blob_file.c
    ${CMAKE_SOURCE_DIR}/frontend/test_blob.c
    ${CMAKE_SOURCE_DIR}/src/minimal_websocket/src/minimal_websocket.c
)

# Debug build
add_executable(test_blob_debug ${SOURCE_FILES})
target_compile_definitions(test_blob_debug PRIVATE DEBUG=1 BLOB_WEBSOCKETS=1)
target_link_libraries(test_blob_debug OpenSSL::SSL OpenSSL::Crypto websockets)
set_target_properties(test_blob_debug PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Release build
add_executable(test_blob_release ${SOURCE_FILES})
target_compile_definitions(test_blob_release PRIVATE BLOB_WEBSOCKETS=1)
target_link_libraries(test_blob_release OpenSSL::SSL OpenSSL::Crypto websockets)
set_target_properties(test_blob_release PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Clean custom target
add_custom_target(clean-cmake-files
   COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
)

# Add a script to clean up all the files that this script generates
file(WRITE clean-all.cmake "
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR})
file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/bin)
")