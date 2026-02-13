#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#define Sleep(ms) usleep((ms) * 1000)
#endif

#include "blob.h"
#include "blob_udp.h"

#define TEST_DURATION_MS 20000
#define SAMPLE_INTERVAL_MS 20
#define TRIANGLE_MAX 100
#define PAYLOAD_SIZE 1000

typedef struct {
    blob_udp *p_udp;
    blob *p_blob;
    volatile int b_running;
    int error_count;
    int max_fragments;
} thread_ctx;

#ifdef _WIN32
DWORD WINAPI sender_thread(LPVOID lpParam) {
#else
void* sender_thread(void *lpParam) {
#endif
    thread_ctx *ctx = (thread_ctx*)lpParam;
    int tick = 0;
    float payload[PAYLOAD_SIZE];

    printf("Sender thread started...\n");
    while (ctx->b_running) {
        if (blob_start(ctx->p_blob, "root") != BLOB_OK) break;
        
        float val = (float)(tick % TRIANGLE_MAX);
        blob_int_a(ctx->p_blob, "tick", &tick, 1);
        blob_float_a(ctx->p_blob, "triangle", &val, 1);
        
        // Add large payload to force fragmentation
        for (int i=0; i<PAYLOAD_SIZE; i++) payload[i] = val + i;
        blob_float_a(ctx->p_blob, "payload", payload, PAYLOAD_SIZE);
        
        if (blob_flush(ctx->p_blob) != BLOB_OK) break;

        tick++;
        Sleep(SAMPLE_INTERVAL_MS);
    }
    printf("Sender thread exiting...\n");
    return 0;
}

#ifdef _WIN32
DWORD WINAPI receiver_thread(LPVOID lpParam) {
#else
void* receiver_thread(void *lpParam) {
#endif
    thread_ctx *ctx = (thread_ctx*)lpParam;
    printf("Receiver thread started...\n");
    
    while (ctx->b_running) {
        if (blob_retrieve_start(ctx->p_blob, "root") == BLOB_OK) {
            int rec_tick = -1;
            float rec_val = -1;
            const float *p_rec_payload = NULL;
            int n_rec_payload = 0;

            // Note: C API needs const int**
            
            const int *p_tick_ptr = NULL;
            int n_tick = 0;
            if (blob_retrieve_int_a(ctx->p_blob, "tick", &p_tick_ptr, &n_tick, 0) == BLOB_OK && p_tick_ptr) {
                rec_tick = *p_tick_ptr;
            }

            const float *p_val_ptr = NULL;
            int n_val = 0;
            if (blob_retrieve_float_a(ctx->p_blob, "triangle", &p_val_ptr, &n_val, 0) == BLOB_OK && p_val_ptr) {
                rec_val = *p_val_ptr;
            }

            if (blob_retrieve_float_a(ctx->p_blob, "payload", &p_rec_payload, &n_rec_payload, 0) == BLOB_OK && p_rec_payload) {
                // Check integrity
                float expected_val = (float)(rec_tick % TRIANGLE_MAX);
                if (rec_val != expected_val) {
                    printf("Validation Failed: Tick %d: Value %f != Expected %f\n", rec_tick, rec_val, expected_val);
                    ctx->error_count++;
                }

                if (n_rec_payload != PAYLOAD_SIZE) {
                    printf("Validation Failed: Payload size %d != Expected %d\n", n_rec_payload, PAYLOAD_SIZE);
                    ctx->error_count++;
                } else {
                    for (int i=0; i<PAYLOAD_SIZE; i++) {
                        if (p_rec_payload[i] != rec_val + i) {
                            printf("Validation Failed: Payload[%d] = %f != Expected %f\n", i, p_rec_payload[i], rec_val + i);
                            ctx->error_count++;
                            break;
                        }
                    }
                }
            }

            int frags = blob_udp_get_n_fragments(ctx->p_udp);
            if (frags > ctx->max_fragments) ctx->max_fragments = frags;
        }
        Sleep(10);
    }
    printf("Receiver thread exiting...\n");
    return 0;
}

int main(int argc, char **argv) {
    blob_udp *p_udp_tx = NULL;
    blob_udp *p_udp_rx = NULL;
    blob_comm_cfg cfg_tx, cfg_rx;
    blob *p_blob_tx = NULL;
    blob *p_blob_rx = NULL;
    thread_ctx tx_ctx, rx_ctx;

    printf("Starting Enhanced Triangle Wave Test...\n");

    // Initialize TX - Sending to core-server (3456)
    if (blob_udp_init(&p_udp_tx, &cfg_tx, "127.0.0.1", 3456, 0, 10) != BLOB_OK) return 1;
    if (blob_init(&p_blob_tx, &cfg_tx) != BLOB_OK) return 1;

    // Initialize RX - Listening on a different port (3457) and sending to itself loopback
    // But blob_udp_init dest port is for SENDING. We want to RECEIVE data from another source?
    // In our run_all_tests, we just want to verify loopback.
    // Let's make the SENDER send to 3457 as well if we want RX to see it.
    // Or just make another sender.
    // Actually, let's make the RX instance listen on 3456 and TX send to 3456.
    // But we usually have core-server on 3456.
    // Let's use 3458 for loopback test to avoid conflicts.
    
    if (blob_udp_init(&p_udp_rx, &cfg_rx, NULL, 0, 3458, 10) != BLOB_OK) return 1;
    if (blob_init(&p_blob_rx, &cfg_rx) != BLOB_OK) return 1;

    // Update TX to send to 3458 so RX can see it
    blob_udp_close(&p_udp_tx);
    blob_close(&p_blob_tx);
    if (blob_udp_init(&p_udp_tx, &cfg_tx, "127.0.0.1", 3458, 0, 10) != BLOB_OK) return 1;
    if (blob_init(&p_blob_tx, &cfg_tx) != BLOB_OK) return 1;

    tx_ctx.p_udp = p_udp_tx;
    tx_ctx.p_blob = p_blob_tx;
    tx_ctx.b_running = 1;
    tx_ctx.error_count = 0;
    tx_ctx.max_fragments = 0;

    rx_ctx.p_udp = p_udp_rx;
    rx_ctx.p_blob = p_blob_rx;
    rx_ctx.b_running = 1;
    rx_ctx.error_count = 0;
    rx_ctx.max_fragments = 0;

#ifdef _WIN32
    HANDLE hSender = CreateThread(NULL, 0, sender_thread, &tx_ctx, 0, NULL);
    HANDLE hReceiver = CreateThread(NULL, 0, receiver_thread, &rx_ctx, 0, NULL);
#else
    pthread_t tSender, tReceiver;
    pthread_create(&tSender, NULL, sender_thread, &tx_ctx);
    pthread_create(&tReceiver, NULL, receiver_thread, &rx_ctx);
#endif

    int duration_ms = TEST_DURATION_MS;
    if (argc > 1) {
        duration_ms = atoi(argv[1]);
    }

    printf("Test running for %d ms...\n", duration_ms);
    Sleep(duration_ms);

    tx_ctx.b_running = 0;
    rx_ctx.b_running = 0;

#ifdef _WIN32
    WaitForSingleObject(hSender, INFINITE);
    WaitForSingleObject(hReceiver, INFINITE);
    CloseHandle(hSender);
    CloseHandle(hReceiver);
#else
    pthread_join(tSender, NULL);
    pthread_join(tReceiver, NULL);
#endif

    printf("\n--- Test Results ---\n");
    printf("Validation Errors: %d\n", rx_ctx.error_count);
    printf("Max Fragments Per Packet: %d\n", rx_ctx.max_fragments);

    int final_ret = 0;
    if (rx_ctx.error_count > 0) {
        printf("FAILED: Data integrity errors detected.\n");
        final_ret = 1;
    }
    if (rx_ctx.max_fragments <= 1) {
        printf("FAILED: Fragmentation not detected (Max fragments: %d). Payload size likely too small.\n", rx_ctx.max_fragments);
        final_ret = 2;
    }

    blob_close(&p_blob_tx);
    blob_udp_close(&p_udp_tx);
    blob_close(&p_blob_rx);
    blob_udp_close(&p_udp_rx);

    if (final_ret == 0) printf("PASSED: Fragmentation and integrity verified.\n");

    return final_ret;
}
