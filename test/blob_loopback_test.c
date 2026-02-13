#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#include "blob.h"
#include "blob_udp.h"
#include "blob_ws_win.h"

// Configuration
#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 3456
#define WS_PORT 8000
#define PAYLOAD_SIZE 5000 /* Enough to force fragmentation */
#define REDUCED_PAYLOAD 1000

volatile int g_b_running = 1;
volatile int g_b_success = 0;

// UDP Sender Thread
unsigned __stdcall sender_thread(void *arg)
{
    printf("[Sender] Thread started. Waiting 2s for WS connection to stabilize...\n");
    Sleep(2000);

    blob_udp *p_udp = NULL;
    blob_comm_cfg cfg;
    blob *p_blob = NULL;
    
    // Initialize UDP sender to core-server
    if (blob_udp_init(&p_udp, &cfg, TARGET_IP, TARGET_PORT, 0, 10) != BLOB_OK) {
        printf("[Sender] Failed to initialize UDP\n");
        return 1;
    }
    
    if (blob_init(&p_blob, &cfg) != BLOB_OK) {
        printf("[Sender] Failed to initialize blob\n");
        return 1;
    }

    printf("[Sender] Sending massive fragmented blob...\n");

    // Start blob
    if (blob_start(p_blob, "loopback_test") != BLOB_OK) {
        printf("[Sender] Failed to start blob\n");
        return 1; 
    }

    // Flush/Send
    printf("[Sender] flushing blob 10 times to fill Jitter Buffer...\n");
    float *big_data = malloc(REDUCED_PAYLOAD * sizeof(float));
    for (int i = 0; i < REDUCED_PAYLOAD; i++) big_data[i] = (float)i;

    for (int k = 0; k < 10; k++) {
        // Reset/init? blob_flush resets. 
        // We need to validify if it clears header etc.
        // blob_start logic? 
        // udp_sender calls blob_start inside loop.
        // We should too.
        
        if (blob_start(p_blob, "loopback_test") != BLOB_OK) {
             printf("Failed blob_start\n");
             break;
        }

        blob_int_a(p_blob, "seq", &k, 1);
        blob_float_a(p_blob, "loopback_data", big_data, REDUCED_PAYLOAD);
        
        if (blob_flush(p_blob) != BLOB_OK) {
             printf("[Sender] Failed to flush blob %d\n", k);
             return 1;
        }
        Sleep(50);
    }
    free(big_data);

    printf("[Sender] Blob sent! Waiting before close...\n");
    Sleep(1000);
    
    // Clean up
    blob_close(&p_blob);
    blob_udp_close(&p_udp);
    
    return 0;
}

int main(int argc, char **argv)
{
    printf("[Main] Starting Loopback Test\n");

    // Initialize internal Blob dependencies?
    // ...

    // Start Sender Thread
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, sender_thread, NULL, 0, NULL);

    // Initialize WS Client
    blob_comm_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    
    if (blob_ws_win_init(&cfg, "127.0.0.1", WS_PORT) != 0) {
        printf("[Main] WS Init failed\n");
        return -1;
    }

    // Initialize Receiver Blob
    blob *p_rx_blob = NULL;
    if (blob_init(&p_rx_blob, &cfg) != BLOB_OK) {
        printf("[Main] Blob Init failed\n");
        return -1;
    }

    printf("[Main] WS Client Initialized. Service Loop Starting...\n");

    DWORD start_time = GetTickCount();

    while (g_b_running) {
        blob_ws_win_service(&cfg);
        
        // Attempt to retrieve blob
        if (blob_retrieve_start(p_rx_blob, "loopback_test") == BLOB_OK) {
            printf("[Main] Received 'loopback_test' blob. Verifying verification...\n");
            
            const float *p_floats = NULL;
            int n_floats = 0;
            
            if (blob_retrieve_float_a(p_rx_blob, "loopback_data", &p_floats, &n_floats, 0) == BLOB_OK) {
                if (n_floats == REDUCED_PAYLOAD) {
                    int errors = 0;
                    for (int i=0; i<n_floats; i++) {
                         if (p_floats[i] != (float)i) {
                              if (errors < 5) printf("Error at %d: %f != %f\n", i, p_floats[i], (float)i);
                              errors++;
                         }
                    }
                    if (errors == 0) {
                        printf("[Main] CONTENT VERIFIED! %d floats match 0..N\n", n_floats);
                        g_b_success = 1;
                        g_b_running = 0;
                    } else {
                        printf("[Main] Data corruption: %d errors.\n", errors);
                    }
                } else {
                    printf("[Main] Count mismatch. Expected %d, got %d\n", REDUCED_PAYLOAD, n_floats);
                }
            } else {
                 printf("[Main] Failed to retrieve loopback_data from valid blob\n");
            }
        }
        
        Sleep(10);
        
        if (GetTickCount() - start_time > 15000) {
            printf("[Main] Timeout waiting for data.\n");
            g_b_running = 0;
        }
    }

    blob_close(&p_rx_blob);
    blob_ws_win_terminate(&cfg);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    if (g_b_success) {
        printf("TEST PASSED\n");
        return 0;
    } else {
        printf("TEST FAILED\n");
        return 1;
    }
}

#else
int main() { return 0; } // Skip on non-windows
#endif
