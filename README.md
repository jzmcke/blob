# Blob Library

A lightweight, high-performance C library for serializing, transmitting, and reconstructing hierarchical data structures ("blobs") over varied transport layers (UDP, WebSocket, etc.). It is designed for real-time telemetry and control in embedded systems and high-throughput web applications.

## Architecture

The Blob library is built on a layered architecture that separates data structure management from serialization and transport.

### Core Components

1.  **Blob Node Tree (`blob_node.h`)**:
    *   Represents data as a hierarchical tree of nodes.
    *   Each node can contain primitive data arrays (int, float, uint) and child nodes.
    *   Supports dynamic construction and traversal.

2.  **Serialization/Core (`blob_core.h`)**:
    *   Handles the flattening of the node tree into a compact binary format.
    *   Handles reconstruction of the tree from binary data.
    *   optimized for minimal overhead.

3.  **Jitter Buffer (`blob_jbuf_frag.h`)**:
    *   Manages packet reordering, de-duplication, and frame assembly.
    *   Critical for ensuring data integrity over unreliable transport protocols like UDP.

4.  **Transport Layers**:
    *   **UDP (`blob_udp.h`)**: Fast, connectionless transmission. Used for high-frequency telemetry.
    *   **WebSocket (`blob_ws_win.c` / `blob_wasm.c`)**: Reliable, full-duplex communication for web clients.
    *   **Frag/Defrag**: Handles splitting large blobs into MTU-sized chunks and reassembling them.

### Data Flow Diagram

```mermaid
graph TD
    subgraph "Producer (C/C++)"
        A[Application Data] --> B[Blob Node Tree]
        B -->|Serialize| C[Binary Blob]
        C -->|Fragment| D[UDP/WS Packets]
    end

    D -->|Network Transmission| E[Receiver]

    subgraph "Consumer (C/C++/WASM/JS)"
        E -->|De-Jitter & Reassemble| F[Binary Blob]
        F -->|Deserialize| G[Blob Node Tree]
        G --> H[Application Usage]
    end
```

## Supported Backends & Platforms

The library is designed to be portable and supports multiple backends:

| Backend | Purpose | Status |
| :--- | :--- | :--- |
| **C (Window/Linux)** | Core library, Desktop Clients, Embedded Systems | ✅ Production Ready |
| **WASM (Emscripten)** | High-performance Browser Decoding | ✅ Integrated |
| **JavaScript (Legacy)** | Legacy Browser Decoding (slower) | ⚠️ Maintenance Mode |
| **Python** | Analysis & Scripting (via CFFI/ctypes) | ✅ Experimental |

## integration & Testing Matrix

We ensure stability through a comprehensive suite of automated tests. Run `python run_all_tests.py` to execute the full suite.

| Test Suite | Description | Components Tested |
| :--- | :--- | :--- |
| **Unit Tests** (`blob_unit_test`) | Verifies core serialization, node manipulation, and jitter buffer logic. | `blob_core`, `blob_node`, `blob_jbuf` |
| **Loopback Test** (`blob_loopback_test`) | End-to-End test sending data from C -> Server -> C (WebSocket). Verifies transport and header handling. | `blob_ws`, `blob_server`, `IP Headers` |
| **Triangle Test** (`blob_triangle_test`) | Simulation of a triangular data generator and receiver to verify signal integrity. | `blob_udp`, Fragmentation |
| **Network Test** (`blob_network_test`) | Basic UDP connectivity and packet loss resilience. | `blob_udp` |
| **JS/WASM Tests** | Verifies browser-side decoding logic (Unit tests via Jest/Mocha). | `blob.js`, `blob_wasm` |

## WebAssembly (WASM) Integration

The WASM module (`blob_wasm.c`) provides a high-performance alternative to the JavaScript decoder. It allows the browser to:
1.  Receive raw UDP/WebSocket packets directly.
2.  Pass them into a C-based Jitter Buffer compiled to WASM.
3.  Reassemble fragments and decode the Blob Node Tree in WASM memory.
4.  Expose the structured data to JavaScript via a zero-copy (or minimal-copy) API.

### Build WASM
To build the WASM module, ensure Emscripten is active and run:
```bash
cd wasm
./build.ps1
```

## Directory Structure

*   `src/`: Core C source files.
*   `include/`: Public API headers.
*   `test/`: C unit and integration tests.
*   `wasm/`: Source and build scripts for the WebAssembly module.
*   `js/`: legacy JavaScript implementation.
*   `python_examples/`: Python bindings and examples.

## Usage Examples

### C / C++ (Producing Data - UDP)

Instead of manual callbacks, use the built-in UDP backend to transmit blobs.

```c
#include "blob.h"
#include "blob_udp.h"

int main() {
    blob_comm_cfg cfg;
    blob_udp *p_udp = NULL;
    blob *p_blob = NULL;

    // 1. Initialize UDP backend (populated 'cfg' with callbacks)
    blob_udp_init(&p_udp, &cfg, "127.0.0.1", 3456, 0, 0);
    
    // 2. Initialize Blob with the UDP config
    blob_init(&p_blob, &cfg);
    
    // 3. Populate and Send
    blob_start(p_blob, "sensor_data");
    float temperature[] = { 23.5f, 24.1f };
    blob_float_a(p_blob, "temp", temperature, 2);
    
    // Serializes and triggers the UDP backend send
    blob_flush(p_blob);
    
    // Cleanup
    blob_close(&p_blob);
    blob_udp_close(&p_udp);
    return 0;
}
```

### C / C++ (Consuming Data - WebSocket)

```c
#include "blob.h"
#include "blob_ws_win.h"

int main() {
    blob_comm_cfg cfg;
    blob *p_blob = NULL;

    // 1. Initialize Windows-native WebSocket backend
    blob_ws_win_init(&cfg, "localhost", 8000);
    blob_init(&p_blob, &cfg);
    
    while (1) {
        // 2. Service the connection (non-blocking)
        blob_ws_win_service(&cfg);
        
        // 3. Try to retrieve a specific node
        if (blob_retrieve_start(p_blob, "loopback_test") == 0) {
            const float *data;
            int n;
            blob_retrieve_float_a(p_blob, "test_var", &data, &n, 0);
            printf("Received %d samples\n", n);
            blob_retrieve_flush(p_blob);
        }
        Sleep(10);
    }
}
```

### Python (Producing Data - UDP)

```python
import numpy as np
from blob.blob_write import BlobWriter
from blob.blob_udp import BlobUDPTx

# 1. Initialize Writer & Transport
writer = BlobWriter('sensor_data', ['temp'])
udp_tx = BlobUDPTx('127.0.0.1', port=3456)

# 2. Update Data & Send
writer.temp = np.array([23.5, 24.1], dtype=np.float32)
data = writer.flush() # Serialize
udp_tx.send(data)     # Transmit
```

### Python (Consuming Data - UDP)

```python
from blob.blob_udp import BlobUDPRx
from blob.blob_read import BlobReader

def on_data(binary_data, addr):
    # reader handles the tree reconstruction and flattening
    flat_data = BlobReader.read(binary_data)
    print(f"Received from {addr}: {flat_data}")

# 1. Initialize Receiver with a callback
udp_rx = BlobUDPRx(port=3456, rx_callback=on_data)

# 2. Start the non-blocking receive thread
udp_rx.start()
```

### JavaScript / WASM (Consuming Data - WebSocket)

```javascript
import { initBlobWasm, decodeBlob } from './blob_wasm_integration.js';

await initBlobWasm();

const ws = new WebSocket('ws://localhost:8000');
ws.binaryType = 'arraybuffer';

ws.onmessage = (event) => {
    // decodeBlob reassembles packets and decodes the tree using WASM
    const nodes = decodeBlob(event.data);
    
    nodes.forEach(node => {
        console.log(`Node: ${node.nodename}`, node.data);
    });
};
```
