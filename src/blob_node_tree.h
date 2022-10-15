/* Node tree is responsible for creating new tree nodes and maintaining a blob of memory
   that serializes the entire node tree. Entire node trees could be buffered in a jitterbuffer by another process.*/
#include <stddef.h>

typedef struct blob_node_tree_retrieve_s blob_node_tree_retrieve;
typedef struct blob_node_tree_send_s blob_node_tree_send;

typedef struct blob_nts_cfg_s
{
    int       (*p_send_cb)(void*, unsigned char*, size_t);
    void       *p_send_context;
} blob_nts_cfg;

typedef struct blob_ntr_cfg_s
{
    int       (*p_rcv_cb)(void*, unsigned char**, size_t*);
    void       *p_rcv_context;
} blob_ntr_cfg;

int
blob_node_tree_send_init(blob_node_tree_send **pp_nts, blob_nts_cfg *p_blob_nts_cfg);

int
blob_node_tree_retrieve_init(blob_node_tree_retrieve **pp_nts, blob_ntr_cfg *p_blob_ntr_cfg);

int
blob_node_tree_send_start(blob_node_tree_send *p_nts, const char *node_name);

int
blob_node_tree_send_flush(blob_node_tree_send *p_nts);

int
blob_node_tree_retrieve_start(blob_node_tree_retrieve *p_ntr, const char *p_node_name);

int
blob_node_tree_retrieve_flush(blob_node_tree_retrieve *p_ntr);

int
blob_node_tree_retrieve_int_a(blob_node_tree_retrieve *p_ntr, const char *var_name, const int **pp_var_val, int *p_n, int rep);
int
blob_node_tree_retrieve_float_a(blob_node_tree_retrieve *p_ntr, const char *var_name, const float **pp_var_val, int *p_n, int rep);
int
blob_node_tree_retrieve_unsigned_int_a(blob_node_tree_retrieve *p_ntr, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep);

int
blob_node_tree_float_a(blob_node_tree_send *p_nts, const char *var_name, float *p_var_val, int n);
int
blob_node_tree_int_a(blob_node_tree_send *p_nts, const char *var_name, int *p_var_val, int n);
int
blob_node_tree_unsigned_int_a(blob_node_tree_send *p_nts, const char *var_name, unsigned int *p_var_val, int n);