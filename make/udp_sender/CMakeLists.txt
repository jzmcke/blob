cmake_minimum_required(VERSION 3.10)
project(udp_sender)

# Source files
set(SOURCES
    ../../test/udp_sender.c
)

# Include directories
include_directories(
    ../../include
    ../../src
)

# Create executable
add_executable(udp_sender ${SOURCES})
set_target_properties(udp_sender PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Link libraries
target_link_libraries(udp_sender blob_core blob_jbuf_lib)

if(WIN32)
    target_link_libraries(udp_sender ws2_32)
else()
    target_link_libraries(udp_sender pthread m)
endif()
