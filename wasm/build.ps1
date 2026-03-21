# Build script for compiling blob decoder to WebAssembly (Windows)

Write-Host "Building blob WASM module..."

# Emscripten compiler
$EMCC = "emcc"

# Source files
$SOURCES = @(
    "blob_wasm.c",
    "../src/blob_core.c",
    "../src/blob_jbuf_frag.c",
    "../src/blob_node_tree.c",
    "../src/blob_node.c",
    "../src/packet.c"
) -join " "

# Include directories
$INCLUDES = "-I../include -I../src"

# Exported functions
$EXPORTS = "['_wasm_blob_init','_wasm_blob_process_packet','_wasm_blob_pull_packet','_wasm_blob_get_ready_count','_wasm_blob_cleanup','_wasm_blob_decode','_wasm_blob_node_free','_wasm_blob_node_get_name','_wasm_blob_node_get_n_children','_wasm_blob_node_get_child','_wasm_blob_node_get_n_vars','_wasm_blob_node_get_var_name','_wasm_blob_node_get_var_data_float','_wasm_blob_node_get_var_data_int','_wasm_blob_node_get_var_data_uint','_wasm_blob_node_get_var_type','_wasm_blob_node_get_var_len','_malloc','_free']"

# Exported runtime methods
$RUNTIME_METHODS = "['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8','getValue','setValue','HEAPU8']"

# Compiler flags
$CFLAGS = "-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -DEMSCRIPTEN"

# Build command
$cmd = "$EMCC $CFLAGS $INCLUDES -s EXPORTED_FUNCTIONS=`"$EXPORTS`" -s EXPORTED_RUNTIME_METHODS=`"$RUNTIME_METHODS`" -s MODULARIZE=1 -s EXPORT_NAME=`"'BlobWASM'`" $SOURCES -o blob_wasm.js"

Write-Host "Running: $cmd"
Invoke-Expression $cmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build complete! Generated:" -ForegroundColor Green
    Write-Host "  - blob_wasm.js"
    Write-Host "  - blob_wasm.wasm"
} else {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}
