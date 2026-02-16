cmake_minimum_required(VERSION 3.10)

project(audio_recorder)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../wav-exporter/include)

# Source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/blob_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/blob_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/blob_node_tree.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/blob.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/blob_ws_win.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/packet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wav-exporter/src/wav_exporter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../test/audio_recorder.c
)

# Executable
add_executable(audio_recorder ${SOURCE_FILES})

# Link libraries
target_link_libraries(audio_recorder PRIVATE ws2_32 wininet)

target_compile_definitions(audio_recorder PRIVATE 
    BLOB_WIN_WS=1 
    _CRT_SECURE_NO_WARNINGS 
    WIN32_LEAN_AND_MEAN 
    _WIN32_WINNT=0x0600
)
