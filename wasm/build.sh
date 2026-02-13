#!/bin/bash
# Build script for compiling blob decoder to WebAssembly

set -e

echo "Building blob WASM module..."

# Emscripten compiler
EMCC=emcc

# Source files
SOURCES="blob_wasm.c \
  ../src/blob_core.c \
  ../src/blob_jbuf_frag.c \
  ../src/blob_node_tree.c \
  ../src/blob_node.c \
  ../src/packet.c"

# Include directories
INCLUDES="-I../include -I../src"

# Exported functions
EXPORTS="[\
'_wasm_blob_init',\
'_wasm_blob_process_packet',\
'_wasm_blob_get_decoded',\
'_wasm_blob_free_string',\
'_wasm_blob_cleanup',\
'_malloc',\
'_free'\
]"

# Exported runtime methods
RUNTIME_METHODS="['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8']"

# Compiler flags
CFLAGS="-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1"

# Build command
$EMCC $CFLAGS \
  $INCLUDES \
  -s EXPORTED_FUNCTIONS="$EXPORTS" \
  -s EXPORTED_RUNTIME_METHODS="$RUNTIME_METHODS" \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="'BlobWASM'" \
  $SOURCES \
  -o blob_wasm.js

echo "Build complete! Generated:"
echo "  - blob_wasm.js"
echo "  - blob_wasm.wasm"
