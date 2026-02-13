#include <stdio.h>
#include <stdlib.h>
#include "blob.h"
#include "blob_udp.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

#define SENDER_PORT 1234
#define RECEIVER_PORT 1235
#define LOOPBACK_ADDR "127.0.0.1"
#define JBUF_LEN 2

int main() {
    printf("Starting Network Tests (blob_udp component)...\n");
    
    blob_udp *p_udp_tx = NULL;
    blob_udp *p_udp_rx = NULL;
    blob_comm_cfg cfg_tx, cfg_rx;
    
    // 1. Init Receiver (local_port 1235)
    if (blob_udp_init(&p_udp_rx, &cfg_rx, NULL, 0, RECEIVER_PORT, JBUF_LEN) != 0) {
        printf("Failed to init UDP receiver\n");
        return 1;
    }
    
    // 2. Init Sender (sending to 1235)
    if (blob_udp_init(&p_udp_tx, &cfg_tx, LOOPBACK_ADDR, RECEIVER_PORT, SENDER_PORT, JBUF_LEN) != 0) {
        printf("Failed to init UDP sender\n");
        return 1;
    }
    
    // 3. Setup Producer (Sender)
    blob *p_blob_tx = NULL;
    blob_init(&p_blob_tx, &cfg_tx);
    
    // 4. Setup Consumer (Receiver)
    blob *p_saved_tx = p_blob_tx;
    
    blob *p_blob_rx = NULL;
    blob_init(&p_blob_rx, &cfg_rx);
    
    // 5. Build and Send Data
    float big_arr[500];
    for(int i=0; i<500; i++) big_arr[i] = (float)i;

    printf("Sending packets...\n");
    fflush(stdout);
    
    for (int i=0; i<10; i++) {
        if (blob_start(p_saved_tx, "root") != BLOB_OK) return 1;
        int val_in_loop = 999;
        blob_int_a(p_saved_tx, "net_int", &val_in_loop, 1);
        blob_float_a(p_saved_tx, "big_arr", big_arr, 500); 

        if (blob_flush(p_saved_tx) != BLOB_OK) return 1;
        
        // No more manual polling! Thread handles it.
        Sleep(10);
    }
    
    // 6. Final wait to ensure all fragments are reassembled
    Sleep(100);

    // 7. Retrieve Data
    // blob_retrieve_start will call cfg_rx.p_rcv_cb -> _blob_udp_rcv_cb -> blob_jbuf_pull
    if (blob_retrieve_start(p_blob_rx, "root") != BLOB_OK) {
        printf("Retrieve start root failed\n");
        fflush(stdout);
        return 1;
    }
    
    printf("Retrieve start root success\n");
    fflush(stdout);

    const int *p_val = NULL;
    int n = 0;
    if (blob_retrieve_int_a(p_blob_rx, "net_int", &p_val, &n, 0) != BLOB_OK) {
        printf("Retrieve net_int failed\n");
        fflush(stdout);
        return 1;
    }
    if (n!=1 || *p_val != 999) {
        printf("Int mismatch: expected 999, got %d, count %d\n", p_val ? *p_val : -1, n);
        fflush(stdout);
        return 1;
    }
    
    const float *p_fval = NULL;
    if (blob_retrieve_float_a(p_blob_rx, "big_arr", &p_fval, &n, 0) != BLOB_OK) {
        printf("Retrieve big_arr failed\n");
        fflush(stdout);
        return 1;
    }
    if (n!=500 || p_fval[499] != 499.0f) {
        printf("Float array mismatch, count %d, last val %f\n", n, p_fval ? p_fval[499] : -1.0f);
        fflush(stdout);
        return 1;
    }
    
    printf("UDP Component Test PASSED\n");
    
    blob_udp_close(&p_udp_tx);
    blob_udp_close(&p_udp_rx);
    return 0;
}
