cmake_minimum_required(VERSION 3.10)

# To execute cmake from the command line, use the following command from a build directory created next to CMakelists.txt:
# cmake -G "Visual Studio 16 2019" -A x64 ..
# Then to build the project, use the following command:
# cmake --build . --config Release

# Set the project name and version
project(mylib VERSION 1.0.0 LANGUAGES C)

# Set the C standard to C99
set(CMAKE_C_STANDARD 99)

set(SOURCES ../../src/blob_jbuf_frag.c ../../src/packet.c)

# Create a shared library
add_library(blob_jbuf_lib SHARED ${SOURCES})

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# Set the output directory for the built library
set_target_properties(blob_jbuf_lib PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
                      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Optional: Define an install target
install(TARGETS blob_jbuf_lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

if (WIN32)
        target_compile_definitions(blob_jbuf_lib PRIVATE BLOB_WINDOWS_DLL)
endif (WIN32)
