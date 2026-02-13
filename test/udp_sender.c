#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define Sleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define Sleep(ms) usleep((ms) * 1000)
#endif

#include "blob.h"
#include "blob_udp.h"

#define PAYLOAD_SIZE 1000  // Large enough to force fragmentation
#define TRIANGLE_MAX 100

int main(int argc, char **argv) {
    blob_udp *p_udp = NULL;
    blob_comm_cfg cfg;
    blob *p_blob = NULL;
    
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int server_port = (argc > 2) ? atoi(argv[2]) : 3456;
    
    printf("UDP Blob Sender - Sending to %s:%d\n", server_ip, server_port);
    printf("This will send fragmented triangle wave data to the server.\n");
    printf("Open browser to http://localhost:8080 to visualize.\n\n");
    
    // Initialize UDP sender to core-server
    if (blob_udp_init(&p_udp, &cfg, server_ip, server_port, 0, 10) != BLOB_OK) {
        printf("Failed to initialize UDP\n");
        return 1;
    }
    
    if (blob_init(&p_blob, &cfg) != BLOB_OK) {
        printf("Failed to initialize blob\n");
        return 1;
    }
    
    printf("Sending triangle wave data (Ctrl+C to stop)...\n");
    
    int tick = 0;
    float payload[PAYLOAD_SIZE];
    
    while (1) {
        // Start blob
        if (blob_start(p_blob, "triangle_wave") != BLOB_OK) break;
        
        // Triangle wave value
        float triangle_val = (float)(tick % TRIANGLE_MAX);
        
        // Add data
        blob_int_a(p_blob, "tick", &tick, 1);
        blob_float_a(p_blob, "triangle", &triangle_val, 1);
        
        // Create a large payload (e.g., 20 KB => multiple fragments)
    // We send this every tick, but maybe we only want to send it occasionally?
    // For now, let's send it every tick to stress test.
    
    // float *big_data = NULL;
    // int big_count = 5000; // 5000 floats * 4 bytes = 20 KB
    // blob_float_a(p_blob, "big_array", big_data, big_count);
    
    // 2026-02-13: Re-enabling fragmentation test
    float big_data[5000];
    for(int i=0; i<5000; i++) big_data[i] = (float)i;
    blob_float_a(p_blob, "big_array", big_data, 5000);
        
        // Send
        if (blob_flush(p_blob) != BLOB_OK) break;
        
        if (tick % 50 == 0) {
            printf("Sent tick %d (triangle value: %.1f)\n", tick, triangle_val);
        }
        
        tick++;
        Sleep(20);  // 50 Hz
    }
    
    printf("\nCleaning up...\n");
    blob_close(&p_blob);
    blob_udp_close(&p_udp);
    
    return 0;
}
