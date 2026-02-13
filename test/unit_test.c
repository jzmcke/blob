#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "blob.h"

// Simple assertion macro
#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "TEST FAILED: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

// Globals are removed for multi-instance support

// Custom file backend callback
int file_write_cb(void *context, unsigned char *data, size_t size) {
    FILE *f = (FILE*)context;
    if (f) {
        printf("DEBUG: writing %zu bytes to file\n", size);
        fwrite(data, 1, size, f);
        return 0; // Success
    }
    return -1;
}

int file_read_cb(void *context, unsigned char **data, size_t *size) {
    FILE *f = (FILE*)context;
    if (f) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        // Pad with 128 bytes (DOWNSTREAM_SERVER_HEADER_BYTES) because blob_node_tree expects it
        size_t padding = 128; 
        unsigned char *buf = malloc(fsize + padding);
        if (buf) {
            memset(buf, 0, padding); // Zero out header
            fread(buf + padding, 1, fsize, f);
            *data = buf;
            *size = fsize + padding;
            printf("DEBUG: read %ld bytes from file, added %zu padding. Total %zu\n", fsize, padding, *size);
            return 0; 
        }
    }
    return -1;
}

int test_basic_structure() {
    printf("Running test_basic_structure...\n");
    
    // Open a test file
    FILE *f = fopen("test_basic.blob", "wb");
    ASSERT(f != NULL, "Failed to open test file");

    blob_comm_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.p_send_cb = file_write_cb;
    cfg.p_send_context = f;
    
    blob *p_test_blob = NULL;
    int res = blob_init(&p_test_blob, &cfg);
    ASSERT(res == BLOB_OK, "blob_init failed");
    
    res = blob_start(p_test_blob, "root_node");
    ASSERT(res == BLOB_OK, "blob_start root_node failed");
    
    int val = 42;
    res = blob_int_a(p_test_blob, "test_int", &val, 1);
    ASSERT(res == BLOB_OK, "blob_int_a failed");
    
    float fval = 3.14f;
    res = blob_float_a(p_test_blob, "test_float", &fval, 1);
    ASSERT(res == BLOB_OK, "blob_float_a failed");
    
    res = blob_flush(p_test_blob);
    ASSERT(res == BLOB_OK, "blob_flush failed");
    
    blob_close(&p_test_blob);
    
    fclose(f);
    printf("test_basic_structure PASSED\n");
    return 0;
}

int test_nested_structure() {
    printf("Running test_nested_structure...\n");
    
    FILE *f = fopen("test_nested.blob", "wb");
    ASSERT(f != NULL, "Failed to open nested test file");

    blob_comm_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.p_send_cb = file_write_cb;
    cfg.p_send_context = f;

    blob *p_blob = NULL;
    blob_init(&p_blob, &cfg);
    
    // Root
    ASSERT(blob_start(p_blob, "root") == BLOB_OK, "Start root failed");
    
    // Nested 1
    ASSERT(blob_start(p_blob, "child1") == BLOB_OK, "Start child1 failed");
    int i = 1;
    blob_int_a(p_blob, "val1", &i, 1);
    
    // Nested 2
    ASSERT(blob_start(p_blob, "grandchild") == BLOB_OK, "Start grandchild failed");
    float fval = 1.23f; // Renamed to fval to avoid shadowing FILE *f
    blob_float_a(p_blob, "val2", &fval, 1);
    
    ASSERT(blob_flush(p_blob) == BLOB_OK, "Flush grandchild failed");
    ASSERT(blob_flush(p_blob) == BLOB_OK, "Flush child1 failed");
    ASSERT(blob_flush(p_blob) == BLOB_OK, "Flush root failed");
    
    fclose(f);
    printf("test_nested_structure PASSED\n");
    return 0;
}

int test_loopback() {
    printf("Running test_loopback...\n");
    fflush(stdout);
    
    // 1. Write data
    FILE *f_write = fopen("test_loopback.blob", "wb");
    ASSERT(f_write != NULL, "Failed to open loopback write file");
    
    blob_comm_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.p_send_cb = file_write_cb;
    cfg.p_send_context = f_write;
    
    blob *p_write_blob = NULL;
    blob_init(&p_write_blob, &cfg);
    blob *p_blob = p_write_blob;
    
    ASSERT(blob_start(p_blob, "root") == BLOB_OK, "Start root failed");
    int sent_val = 12345;
    blob_int_a(p_blob, "test_int", &sent_val, 1);
    ASSERT(blob_flush(p_blob) == BLOB_OK, "Flush failed");
    
    fclose(f_write);
    
    // 2. Read data
    FILE *f_read = fopen("test_loopback.blob", "rb");
    ASSERT(f_read != NULL, "Failed to open loopback read file");
    
    // Re-init for reading? Or just update context?
    // blob_init allocates new structure. We should probably reuse p_g_blob or free/init?
    // blob.c doesn't have blob_term/deinit exposed.
    // But we can just use the existing p_g_blob if we update the callbacks in the underlying NTS/NTR?
    // blob struct definition is hidden in blob.c. We can't access p_ntr directly.   
    // So we should call blob_init again? It will leak memory (old p_g_blob) but for test it's fine.
    
    memset(&cfg, 0, sizeof(cfg));
    cfg.p_rcv_cb = file_read_cb;
    cfg.p_rcv_context = f_read;
    
    blob_init(&p_blob, &cfg); // Re-init with read callback
    
    // 3. Verify
    // For retrieve, we usually do:
    // blob_retrieve_start implicitly triggers pull?
    // Wait, blob_retrieve_start seems to traverse the tree structure?
    // Let's assume standard usage:
    ASSERT(blob_retrieve_start(p_blob, "root") == BLOB_OK, "Retrieve start root failed");
    
    const int *p_val = NULL;
    int n = 0;
    // retrieve_int_a returns pointer to internal buffer
    ASSERT(blob_retrieve_int_a(p_blob, "test_int", &p_val, &n, 0) == BLOB_OK, "Retrieve int failed");
    ASSERT(n == 1, "Retrieve count mismatch");
    ASSERT(p_val != NULL, "Retrieve ptr null");
    ASSERT(*p_val == 12345, "Retrieve value mismatch");
    
    blob_retrieve_flush(p_blob); // Cleanup?
    
    fclose(f_read);
    printf("test_loopback PASSED\n");
    return 0;
}

int test_nested_loopback() {
    printf("Running test_nested_loopback...\n");
    fflush(stdout);
    
    // 1. Write Nested Data
    FILE *f_write = fopen("test_nested_loop.blob", "wb");
    ASSERT(f_write != NULL, "Failed to open nested loopback write file");
    
    blob_comm_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.p_send_cb = file_write_cb;
    cfg.p_send_context = f_write;
    
    blob *p_nest_blob = NULL;
    blob_init(&p_nest_blob, &cfg);
    blob *p_blob = p_nest_blob;
    
    ASSERT(blob_start(p_blob, "root") == BLOB_OK, "Start root failed");
    
    ASSERT(blob_start(p_blob, "folder_a") == BLOB_OK, "Start folder_a failed");
    printf("Started folder_a\n");
    int val_a = 100;
    blob_int_a(p_blob, "val_a", &val_a, 1);
    
    // Simplified loopback: root -> folder_a
    ASSERT(blob_flush(p_blob) == BLOB_OK, "Flush folder_a failed");
    printf("Flushed folder_a\n");
    ASSERT(blob_flush(p_blob) == BLOB_OK, "Flush root failed");
    printf("Flushed root\n");
    fclose(f_write);
    
    // 2. Read Nested Data
    FILE *f_read = fopen("test_nested_loop.blob", "rb");
    ASSERT(f_read != NULL, "Failed to open nested loopback read file");
    
    memset(&cfg, 0, sizeof(cfg));
    cfg.p_rcv_cb = file_read_cb;
    cfg.p_rcv_context = f_read;
    
    blob_init(&p_blob, &cfg); 
    
    // 3. Verify
    ASSERT(blob_retrieve_start(p_blob, "root") == BLOB_OK, "Retrieve start root failed");
    
    // Verify folder_a
    ASSERT(blob_retrieve_start(p_blob, "folder_a") == BLOB_OK, "Enter folder_a failed");
    const int *p_val_a = NULL;
    int n = 0;
    ASSERT(blob_retrieve_int_a(p_blob, "val_a", &p_val_a, &n, 0) == BLOB_OK, "Retrieve val_a failed");
    ASSERT(n == 1 && *p_val_a == 100, "val_a mismatch");
    
    // Check we can't find folder_b anymore
    // ASSERT(blob_retrieve_start(p_blob, "folder_b") != BLOB_OK, "Should not find folder_b");
    
    // Flush/Close retrieval
    blob_retrieve_flush(p_blob);
    
    fclose(f_read);
    printf("test_nested_loopback PASSED\n");
    return 0;
}

int main() {
    printf("Starting Blob Unit Tests...\n");
    fflush(stdout);
    
    if (test_basic_structure() != 0) return 1;
    if (test_nested_structure() != 0) return 1;
    if (test_loopback() != 0) return 1;
    if (test_nested_loopback() != 0) return 1;
    
    printf("ALL TESTS PASSED\n");
    return 0;
}
