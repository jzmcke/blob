#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <emscripten.h>
#include "../src/blob_jbuf_frag.h"
#include "../src/blob_core.h"
#include "../src/blob_node.h"

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
 */
EMSCRIPTEN_KEEPALIVE
int wasm_blob_process_packet(unsigned char *p_data, size_t len) {
    if (g_jbuf == NULL) {
        return -1; 
    }
    
    unsigned char *packet_copy = (unsigned char*)malloc(len);
    if (!packet_copy) return -1;
    memcpy(packet_copy, p_data, len);
    
    int result = blob_jbuf_push(g_jbuf, packet_copy, len);
    return blob_jbuf_get_n_fragments(g_jbuf);
}

/**
 * Pull the next complete packet from jitter buffer
 */
EMSCRIPTEN_KEEPALIVE
unsigned char* wasm_blob_pull_packet(size_t *p_size) {
    if (g_jbuf == NULL) return NULL;
    void *p_data = NULL;
    size_t n = 0;
    int result = blob_jbuf_pull(g_jbuf, &p_data, &n);
    if (result != BLOB_JBUF_OK || p_data == NULL) {
        if (p_size) *p_size = 0;
        return NULL;
    }
    if (p_size) *p_size = n;
    return (unsigned char*)p_data;
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_get_ready_count() {
    if (g_jbuf == NULL) return 0;
    return blob_jbuf_get_n_fragments(g_jbuf);
}

// --- NEW DECODING FUNCTIONS ---

/**
 * Decode a blob into a node tree.
 * JS owns the returned pointer and must call wasm_blob_node_free.
 */
EMSCRIPTEN_KEEPALIVE
blob_node* wasm_blob_decode(unsigned char *p_data, size_t len) {
    blob_node *p_node = NULL;
    size_t processed_len = 0;
    int result = blob_node_disassemble_data(&p_node, p_data, &processed_len);
    if (result != BLOB_OK) return NULL;
    return p_node;
}

EMSCRIPTEN_KEEPALIVE
void wasm_blob_node_free(blob_node *p_node) {
    if (p_node) {
        blob_node_close(&p_node);
    }
}

EMSCRIPTEN_KEEPALIVE
const char* wasm_blob_node_get_name(blob_node *p_node) {
    return p_node ? p_node->p_name : "";
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_node_get_n_children(blob_node *p_node) {
    return p_node ? p_node->n_children : 0;
}

EMSCRIPTEN_KEEPALIVE
blob_node* wasm_blob_node_get_child(blob_node *p_node, int index) {
    if (!p_node || index < 0 || index >= p_node->n_children) return NULL;
    return p_node->ap_child_nodes[index];
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_node_get_n_vars(blob_node *p_node) {
    if (!p_node || !p_node->p_blob) return 0;
    int *p_var_len, *p_var_types;
    char *p_var_names;
    int n_vars, n_reps;
    blob_core_get_info(p_node->p_blob, &p_var_len, &p_var_types, &p_var_names, &n_vars, &n_reps);
    return n_vars;
}

EMSCRIPTEN_KEEPALIVE
const char* wasm_blob_node_get_var_name(blob_node *p_node, int var_idx) {
    if (!p_node || !p_node->p_blob) return "";
    int *p_var_len, *p_var_types;
    char *p_var_names;
    int n_vars, n_reps;
    blob_core_get_info(p_node->p_blob, &p_var_len, &p_var_types, &p_var_names, &n_vars, &n_reps);
    if (var_idx < 0 || var_idx >= n_vars) return "";
    return p_var_names + (var_idx * BLOB_MAX_VAR_NAME_LEN);
}

EMSCRIPTEN_KEEPALIVE
float* wasm_blob_node_get_var_data_float(blob_node *p_node, const char *var_name, int *p_n) {
    if (!p_node || !p_node->p_blob) return NULL;
    const float *p_data = NULL;
    int n = 0;
    int result = blob_node_retrieve_float_a(p_node, var_name, &p_data, &n, 0);
    if (result != BLOB_OK) return NULL;
    if (p_n) *p_n = n;
    return (float*)p_data;
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_node_get_var_type(blob_node *p_node, int var_idx) {
    if (!p_node || !p_node->p_blob) return -1;
    int *p_var_len, *p_var_types;
    char *p_var_names;
    int n_vars, n_reps;
    blob_core_get_info(p_node->p_blob, &p_var_len, &p_var_types, &p_var_names, &n_vars, &n_reps);
    if (var_idx < 0 || var_idx >= n_vars) return -1;
    return p_var_types[var_idx];
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_node_get_var_len(blob_node *p_node, int var_idx) {
    if (!p_node || !p_node->p_blob) return 0;
    int *p_var_len, *p_var_types;
    char *p_var_names;
    int n_vars, n_reps;
    blob_core_get_info(p_node->p_blob, &p_var_len, &p_var_types, &p_var_names, &n_vars, &n_reps);
    if (var_idx < 0 || var_idx >= n_vars) return 0;
    return p_var_len[var_idx];
}

EMSCRIPTEN_KEEPALIVE
void wasm_blob_cleanup() {
    if (g_jbuf) {
        blob_jbuf_close(&g_jbuf);
        g_jbuf = NULL;
    }
}
