cmake_minimum_required(VERSION 3.10)

# This is the top-level project name
project(blob)

# Add a subdirectory for each of your projects. CMake will look for a CMakeLists.txt file in each of these directories.
add_subdirectory(make/test_blob)
add_subdirectory(make/blob_jbuf_lib)
# Add more as needed