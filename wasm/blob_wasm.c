#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <emscripten.h>
#include "../src/blob_jbuf_frag.h"
#include "../src/blob_frag_tx.h"
#include "../src/blob_core.h"
#include "../src/blob_node.h"

// Deallocate callback for jitter buffer
static void wasm_dealloc_callback(unsigned char *p_data, void *p_context) {
    if (p_data) {
        free(p_data);
    }
}

/**
 * Initialize a new blob WASM decoder with jitter buffer.
 * Returns a handle (pointer) to the jitter buffer instance.
 */
EMSCRIPTEN_KEEPALIVE
blob_jbuf* wasm_blob_init(int jbuf_len) {
    blob_jbuf *p_jbuf = NULL;
    blob_jbuf_cfg cfg = {
        .jbuf_len = jbuf_len,
        .deallocate_callback = wasm_dealloc_callback,
        .p_context = NULL
    };
    
    int result = blob_jbuf_init(&p_jbuf, &cfg);
    if (result != BLOB_JBUF_OK) {
        return NULL;
    }
    
    return p_jbuf;
}

/**
 * Process an incoming WebSocket packet (may be fragmented)
 */
EMSCRIPTEN_KEEPALIVE
int wasm_blob_process_packet(blob_jbuf *p_jbuf, unsigned char *p_data, size_t len) {
    if (p_jbuf == NULL) {
        return -1; 
    }
    
    unsigned char *packet_copy = (unsigned char*)malloc(len);
    if (!packet_copy) return -1;
    memcpy(packet_copy, p_data, len);
    
    int result = blob_jbuf_push(p_jbuf, packet_copy, len);
    return blob_jbuf_get_n_fragments(p_jbuf);
}

/**
 * Pull the next complete packet from jitter buffer
 */
EMSCRIPTEN_KEEPALIVE
unsigned char* wasm_blob_pull_packet(blob_jbuf *p_jbuf, size_t *p_size) {
    if (p_jbuf == NULL) return NULL;
    void *p_data = NULL;
    size_t n = 0;
    int result = blob_jbuf_pull(p_jbuf, &p_data, &n);
    if (result != BLOB_JBUF_OK || p_data == NULL) {
        if (p_size) *p_size = 0;
        return NULL;
    }
    if (p_size) *p_size = n;
    return (unsigned char*)p_data;
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_get_ready_count(blob_jbuf *p_jbuf) {
    if (p_jbuf == NULL) return 0;
    return blob_jbuf_get_n_fragments(p_jbuf);
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
int* wasm_blob_node_get_var_data_int(blob_node *p_node, const char *var_name, int *p_n) {
    if (!p_node || !p_node->p_blob) return NULL;
    const int *p_data = NULL;
    int n = 0;
    int result = blob_node_retrieve_int_a(p_node, var_name, &p_data, &n, 0);
    if (result != BLOB_OK) return NULL;
    if (p_n) *p_n = n;
    return (int*)p_data;
}

EMSCRIPTEN_KEEPALIVE
unsigned int* wasm_blob_node_get_var_data_uint(blob_node *p_node, const char *var_name, int *p_n) {
    if (!p_node || !p_node->p_blob) return NULL;
    const unsigned int *p_data = NULL;
    int n = 0;
    int result = blob_node_retrieve_unsigned_int_a(p_node, var_name, &p_data, &n, 0);
    if (result != BLOB_OK) return NULL;
    if (p_n) *p_n = n;
    return (unsigned int*)p_data;
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
void wasm_blob_cleanup(blob_jbuf *p_jbuf) {
    if (p_jbuf) {
        blob_jbuf_close(&p_jbuf);
    }
}

// --- FRAGMENTATION FOR TRANSMISSION ---

EMSCRIPTEN_KEEPALIVE
blob_frag_tx* wasm_blob_frag_init(int frag_size) {
    blob_frag_tx *p_tx = NULL;
    if (blob_frag_tx_init(&p_tx, frag_size) != BLOB_OK) {
        return NULL;
    }
    return p_tx;
}

EMSCRIPTEN_KEEPALIVE
int wasm_blob_frag_begin(blob_frag_tx *p_tx, unsigned char *p_data, size_t len) {
    if (!p_tx || !p_data) return -1;
    return blob_frag_tx_begin_packet(p_tx, p_data, len);
}

EMSCRIPTEN_KEEPALIVE
unsigned char* wasm_blob_frag_next(blob_frag_tx *p_tx, size_t *p_len) {
    if (!p_tx) return NULL;
    unsigned char *p_chunk = NULL;
    size_t chunk_len = 0;
    int res = blob_frag_tx_next_packet(p_tx, &p_chunk, &chunk_len);
    if (res == BLOB_OK && p_chunk != NULL) {
        if (p_len) *p_len = chunk_len;
        return p_chunk;
    }
    if (p_len) *p_len = 0;
    return NULL;
}

EMSCRIPTEN_KEEPALIVE
void wasm_blob_frag_free(blob_frag_tx *p_tx) {
    if (p_tx) {
        blob_frag_tx_close(&p_tx);
    }
}
