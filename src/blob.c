#include <stdlib.h>

#include "blob_node_tree.h"
#include "blob/include/blob.h"
#include "blob_node.h"

blob_comm_cfg g_blob_ccfg;
blob *p_g_blob;

struct blob_s
{
   blob_node_tree_send     *p_nts;
   blob_node_tree_retrieve *p_ntr;
};

int
blob_init(blob_comm_cfg *p_blob_comm_cfg)
{
    p_g_blob = (blob*)calloc(sizeof(blob), 1);
    {
        blob_nts_cfg nts_cfg;
        nts_cfg.p_send_cb = p_blob_comm_cfg->p_send_cb;
        nts_cfg.p_send_context = p_blob_comm_cfg->p_send_context;
        blob_node_tree_send_init(&p_g_blob->p_nts, &nts_cfg);
    }
    {
        blob_ntr_cfg ntr_cfg;
        ntr_cfg.p_rcv_cb = p_blob_comm_cfg->p_rcv_cb;
        ntr_cfg.p_rcv_context = p_blob_comm_cfg->p_rcv_context;
        blob_node_tree_retrieve_init(&p_g_blob->p_ntr, &ntr_cfg);
    }
    return BLOB_OK;
}

/* Creates the blob file if not already created */
int
blob_start(blob *p_blob, const char *node_name)
{
    return blob_node_tree_send_start(p_blob->p_nts, node_name);
}

/* Appends an array of float values to a blob */
int
blob_float_a(blob *p_blob, const char *var_name, float *p_var_val, int n)
{
    return blob_node_tree_float_a(p_blob->p_nts, var_name, p_var_val, n);
}
/* Appends an array of int values to a blob */
int
blob_int_a(blob *p_blob, const char *var_name, int *p_var_val, int n)
{
    return blob_node_tree_int_a(p_blob->p_nts, var_name, p_var_val, n);
}

/* Appends an array of unsigned int values to a blob */
int
blob_unsigned_int_a(blob *p_blob, const char *var_name, unsigned int *p_var_val, int n)
{
    return blob_node_tree_unsigned_int_a(p_blob->p_nts, var_name, p_var_val, n);
}

/* Returns the data and transmits it via the connection callback */
int
blob_flush(blob *p_blob)
{
    return blob_node_tree_send_flush(p_blob->p_nts);
}

/* Retreives data from the connection and disassembles */
int
blob_retrieve_start(blob *p_blob, const char* node_name)
{
    return blob_node_tree_retrieve_start(p_blob->p_ntr, node_name);
}

/* Appends an array of float values to a blob */
int
blob_retrieve_float_a(blob *p_blob, const char *var_name, const float **pp_var_val, int *p_n, int rep)
{
    return blob_node_tree_retrieve_float_a(p_blob->p_ntr, var_name, pp_var_val, p_n, rep);
}

/* Appends an array of int values to a blob */
int
blob_retrieve_int_a(blob *p_blob, const char *var_name, const int **pp_var_val, int *p_n, int rep)
{
    return blob_node_tree_retrieve_int_a(p_blob->p_ntr, var_name, pp_var_val, p_n, rep);
}

/* Appends an array of unsigned int values to a blob */
int
blob_retrieve_unsigned_int_a(blob *p_blob, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep)
{
    return blob_node_tree_retrieve_unsigned_int_a(p_blob->p_ntr, var_name, pp_var_val, p_n, rep);
}

/* Flushes the memory. */
int
blob_retrieve_flush(blob *p_blob)
{
    return blob_node_tree_retrieve_flush(p_blob->p_ntr);
}
