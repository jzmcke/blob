#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <emscripten.h>
#include "../src/blob_jbuf_frag.h"
#include "../src/blob_core.h"

// Global jitter buffer instance
static blob_jbuf *g_jbuf = NULL;

// Deallocate callback for jitter buffer
static void wasm_dealloc_callback(unsigned char *p_data, void *p_context) {
    if (p_data) {
        free(p_data);
    }
}

/**
 * Initialize the blob WASM decoder with jitter buffer
 * @param jbuf_len Jitter buffer length (number of packets to buffer)
 * @return 0 on success, -1 on error
 */
EMSCRIPTEN_KEEPALIVE
int wasm_blob_init(int jbuf_len) {
    if (g_jbuf != NULL) {
        return 0; // Already initialized
    }
    
    blob_jbuf_cfg cfg = {
        .jbuf_len = jbuf_len,
        .deallocate_callback = wasm_dealloc_callback,
        .p_context = NULL
    };
    
    int result = blob_jbuf_init(&g_jbuf, &cfg);
    if (result != BLOB_JBUF_OK) {
        return -1;
    }
    
    return 0;
}

/**
 * Process an incoming WebSocket packet (may be fragmented)
 * @param p_data Pointer to packet data
 * @param len Length of packet data
 * @return Number of complete packets ready to decode, or negative on error
 */
EMSCRIPTEN_KEEPALIVE
int wasm_blob_process_packet(unsigned char *p_data, size_t len) {
    if (g_jbuf == NULL) {
        return -1; // Not initialized
    }
    
    // Allocate memory for packet (will be freed by jitter buffer)
    unsigned char *packet_copy = (unsigned char*)malloc(len);
    if (!packet_copy) {
        return -1;
    }
    
    memcpy(packet_copy, p_data, len);
    
    // Push packet into jitter buffer
    int result = blob_jbuf_push(g_jbuf, packet_copy, len);
    
    // Return number of fragments ready
    return blob_jbuf_get_n_fragments(g_jbuf);
}

/**
 * Pull the next complete packet from jitter buffer
 * Returns pointer to packet data (must be freed by caller)
 * @param p_size Output parameter for packet size
 * @return Pointer to packet data, or NULL if none available
 */
EMSCRIPTEN_KEEPALIVE
unsigned char* wasm_blob_pull_packet(size_t *p_size) {
    if (g_jbuf == NULL) {
        return NULL;
    }
    
    void *p_data = NULL;
    size_t n = 0;
    
    // Pull complete packet from jitter buffer
    int result = blob_jbuf_pull(g_jbuf, &p_data, &n);
    if (result != BLOB_JBUF_OK || p_data == NULL) {
        if (p_size) *p_size = 0;
        return NULL;
    }
    
    if (p_size) *p_size = n;
    return (unsigned char*)p_data;
}

/**
 * Get number of complete packets ready in jitter buffer
 */
EMSCRIPTEN_KEEPALIVE
int wasm_blob_get_ready_count() {
    if (g_jbuf == NULL) {
        return 0;
    }
    return blob_jbuf_get_n_fragments(g_jbuf);
}

/**
 * Cleanup and free resources
 */
EMSCRIPTEN_KEEPALIVE
void wasm_blob_cleanup() {
    if (g_jbuf) {
        blob_jbuf_close(&g_jbuf);
        g_jbuf = NULL;
    }
}
