cmake_minimum_required(VERSION 3.10)

# This is the top-level project name
project(blob)

# Add a subdirectory for each of your projects. CMake will look for a CMakeLists.txt file in each of these directories.
# add_subdirectory(make/test_blob)
add_subdirectory(make/blob_jbuf_lib)
add_subdirectory(make/unit_test)
add_subdirectory(make/network_test)
add_subdirectory(make/blob_core_lib)
add_subdirectory(make/udp_sender)
# Add more as needed