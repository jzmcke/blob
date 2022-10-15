/* Node is responsible for the namespacing of variables and maintaining the blob structure. */

#include "blob_core.h"

#define MAX_NODENAME_LEN (128)
#define MAX_CHILD_NODES (32)
#define MAX_BLOBS_PER_NODE (32)

typedef struct blob_node_s
{
    char                 p_name[MAX_NODENAME_LEN];  /* Name of this blob node. */
    blob_core           *p_blob; /* For repeated calls inside a node. All blobs should be identical, except for their data.. */
    int                  n_children;
    struct blob_node_s  *ap_child_nodes[MAX_CHILD_NODES];
    struct blob_node_s  *p_parent_node;
    size_t               blob_size; /* Only valid after complete traverse */
} blob_node;

int
blob_node_close(blob_node **pp_blob_node);

int
blob_node_float_a(blob_node *p_node, const char *var_name, float *p_var_val, int n);

int
blob_node_int_a(blob_node *p_node, const char *var_name, int *p_var_val, int n);

int
blob_node_unsigned_int_a(blob_node *p_node, const char *var_name, unsigned int *p_var_val, int n);

int
blob_node_retrieve_float_a(blob_node *p_node, const char *var_name, const float **pp_var_val, int *p_n, int rep);

int
blob_node_retrieve_int_a(blob_node *p_node, const char *var_name, const int **pp_var_val, int *p_n, int rep);

int
blob_node_retrieve_unsigned_int_a(blob_node *p_node, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep);

int
blob_node_retrieve_float_a_default(blob_node *p_node, const char *var_name, const float **pp_var_val, int *p_n, float *p_default);

int
blob_node_retrieve_int_a_default(blob_node *p_node, const char *var_name, const int **pp_var_val, int *p_n, int *p_default);

int
blob_node_retrieve_unsigned_int_a_default(blob_node *p_node, const char *var_name, const unsigned int **pp_var_val, int *p_n, unsigned int *p_default);

int
blob_node_aggregate_data(blob_node *p_node, size_t *p_size);

int
blob_node_assemble_data(blob_node *p_node, unsigned char *p_data, size_t *p_size);

int
blob_node_disassemble_data(blob_node **pp_node, unsigned char *p_data, size_t *p_size);

void
blob_node_get_data(blob_node *p_node, unsigned char **pp_data, size_t *p_base_blob_size);
