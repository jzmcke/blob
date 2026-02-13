# Blob WebAssembly Module

This directory contains the WebAssembly build of the blob decoder.

## Prerequisites

Install Emscripten SDK:

### Windows
```powershell
# Download and install
git clone https://github.com/emscripten-core/emsdk.git C:\emsdk
cd C:\emsdk
.\emsdk install latest
.\emsdk activate latest

# Add to PATH for current session
.\emsdk_env.ps1
```

### Linux/Mac
```bash
# Download and install
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk
./emsdk install latest
./emsdk activate latest

# Add to PATH
source ./emsdk_env.sh
```

## Building

### Windows
```powershell
cd wasm
.\build.ps1
```

### Linux/Mac
```bash
cd wasm
chmod +x build.sh
./build.sh
```

This will generate:
- `blob_wasm.js` - JavaScript glue code
- `blob_wasm.wasm` - WebAssembly binary

## Usage in Browser

```javascript
// Load the WASM module
const BlobModule = await BlobWASM();

// Initialize with jitter buffer length
const jbufLen = 10;
BlobModule.ccall('wasm_blob_init', 'number', ['number'], [jbufLen]);

// Process incoming WebSocket packet
const packet = new Uint8Array([...]); // Your packet data
const ptr = BlobModule._malloc(packet.length);
BlobModule.HEAPU8.set(packet, ptr);
const numReady = BlobModule.ccall('wasm_blob_process_packet', 'number', 
    ['number', 'number'], [ptr, packet.length]);
BlobModule._free(ptr);

// Get decoded blob (if ready)
if (numReady > 0) {
    const jsonPtr = BlobModule.ccall('wasm_blob_get_decoded', 'number', [], []);
    if (jsonPtr) {
        const jsonStr = BlobModule.UTF8ToString(jsonPtr);
        const decoded = JSON.parse(jsonStr);
        BlobModule.ccall('wasm_blob_free_string', null, ['number'], [jsonPtr]);
    }
}

// Cleanup
BlobModule.ccall('wasm_blob_cleanup', null, [], []);
```

## Features

- **Jitter Buffer**: Handles out-of-order packet delivery
- **Fragmentation**: Properly reassembles fragmented UDP packets
- **Single Source of Truth**: Uses the same C code as native clients
- **Performance**: Compiled to WASM for near-native speed
