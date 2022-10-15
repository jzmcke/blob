#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "blob_core.h"
#include "blob_node.h"

int
blob_node_close(blob_node **pp_blob_node)
{
    blob_node *p_blob_node = *pp_blob_node;
    for (int i=0; i<p_blob_node->n_children; i++)
    {
        /* Must free child nodes before attempting to free this node */
        assert(NULL == p_blob_node->ap_child_nodes[i]);
        return BLOB_ERR;
    }

    blob_core_close(&p_blob_node->p_blob);
    free(*pp_blob_node);
    *pp_blob_node = NULL;
    return BLOB_OK;
}


int
blob_node_float_a(blob_node *p_node, const char *var_name, float *p_var_val, int n)
{
    if (NULL == p_node->p_blob)
    {
        /* Create a new blob */
        blob_core_cfg cfg;
        cfg.protocol = BLOB_PROTOCOL_VERSION_FIRST;
        blob_core_init(&p_node->p_blob, &cfg);
    }
    blob_core_float_a(p_node->p_blob, var_name, p_var_val, n);
    return BLOB_OK;
}

int
blob_node_int_a(blob_node *p_node, const char *var_name, int *p_var_val, int n)
{
    if (NULL == p_node->p_blob)
    {
        /* Create a new blob */
        blob_core_cfg cfg;
        cfg.protocol = BLOB_PROTOCOL_VERSION_FIRST;
        blob_core_init(&p_node->p_blob, &cfg);
    }
    blob_core_int_a(p_node->p_blob, var_name, p_var_val, n);
    return BLOB_OK;
}

int
blob_node_unsigned_int_a(blob_node *p_node, const char *var_name, unsigned int *p_var_val, int n)
{
    if (NULL == p_node->p_blob)
    {
        /* Create a new blob */
        blob_core_cfg cfg;
        cfg.protocol = BLOB_PROTOCOL_VERSION_FIRST;
        blob_core_init(&p_node->p_blob, &cfg);
    }
    blob_core_unsigned_int_a(p_node->p_blob, var_name, p_var_val, n);
    return BLOB_OK;
}



int
blob_node_retrieve_float_a(blob_node *p_node, const char *var_name, const float **pp_var_val, int *p_n, int rep)
{

    blob_core_retrieve_float_a(p_node->p_blob, var_name, pp_var_val, p_n, rep);
    return 0;
}

int
blob_node_retrieve_int_a(blob_node *p_node, const char *var_name, const int **pp_var_val, int *p_n, int rep)
{

    blob_core_retrieve_int_a(p_node->p_blob, var_name, pp_var_val, p_n, rep);
    return 0;
}

int
blob_node_retrieve_unsigned_int_a(blob_node *p_node, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep)
{

    blob_core_retrieve_unsigned_int_a(p_node->p_blob, var_name, pp_var_val, p_n, rep);
    return 0;
}

void
blob_node_get_data(blob_node *p_node, unsigned char **pp_data, size_t *p_base_blob_size)
{
    blob_core_get_data(p_node->p_blob, pp_data, p_base_blob_size);
}

int
blob_node_aggregate_data(blob_node *p_node, size_t *p_size)
{
    size_t total_size = 0;

    size_t tmp_size;
    unsigned char *p_data;
    if (NULL != p_node->p_blob)
    {
        blob_core_get_data(p_node->p_blob, &p_data, &tmp_size);
        total_size += tmp_size;
    }
    
    for (int i=0; i<p_node->n_children; i++)
    {
        size_t tmp_size = 0;
        blob_node_aggregate_data(p_node->ap_child_nodes[i], &tmp_size);
        total_size += tmp_size;
    }

    if (NULL != p_node->p_blob)
    {
        blob_core_header_get_size(p_node->p_blob, &tmp_size);
        total_size += tmp_size;
    }
    
    total_size += sizeof(int); /* node has blob ? */
    total_size += sizeof(unsigned int); /* number of children */
    total_size += sizeof(char) * MAX_NODENAME_LEN; /* name of the node */

    p_node->blob_size = total_size;
    *p_size = total_size;
    return 0;
}

int
blob_node_assemble_data(blob_node *p_node, unsigned char *p_data, size_t *p_size)
{
    size_t size_left = *p_size;
    int n_initial_children = p_node->n_children;
    while (p_node->n_children > 0)
    {
        blob_node_assemble_data(p_node->ap_child_nodes[p_node->n_children-1], p_data, &size_left);
        blob_node_close(&p_node->ap_child_nodes[p_node->n_children-1]);
        p_node->n_children--;
    }

    {
        int  *p_var_len;
        int  *p_var_types;
        char *p_var_names;
        int   n_vars;
        int   n_repetitions;
        unsigned char *p_blob_data;
        size_t blob_size;
        int b_node_has_blob;

        /* This node has no children. Copy blob contents over. */
        if (NULL != p_node->p_blob)
        {
            b_node_has_blob = 1;
            blob_core_get_data(p_node->p_blob, &p_blob_data, &blob_size);
            memcpy(p_data + size_left - blob_size, p_blob_data, blob_size);
            size_left -= blob_size;
            /* write the blob header */
            blob_core_get_info(p_node->p_blob,
                               &p_var_len,
                               &p_var_types,
                               &p_var_names,
                               &n_vars,
                               &n_repetitions);
            /* Copy the variable lengths into the buffer */
            memcpy(p_data + size_left - (n_vars * sizeof(int)), p_var_len, n_vars * sizeof(int));
            size_left -= (n_vars * sizeof(int));
            /* Copy the variable of types into the buffer */
            memcpy(p_data + size_left - (n_vars * sizeof(int)), p_var_types, n_vars * sizeof(int));
            size_left -= (n_vars * sizeof(int));
            /* Copy the variable names into the buffer */
            memcpy(p_data + size_left - (n_vars * sizeof(char) * BLOB_MAX_VAR_NAME_LEN), p_var_names, n_vars * sizeof(char) * BLOB_MAX_VAR_NAME_LEN);
            size_left -= (n_vars * sizeof(char) * BLOB_MAX_VAR_NAME_LEN);
            /* Copy the number of variables per blob */
            memcpy(p_data + size_left - sizeof(int), &n_vars, sizeof(int));
            size_left -= sizeof(int);
            /* Copy the number of repetitions of the variables in each blob */
            memcpy(p_data + size_left - sizeof(int), &n_repetitions, sizeof(int));
            size_left -= sizeof(int);
        }
        else
        {
            b_node_has_blob = 0;
            blob_size = 0;
        }
        
        /* Write if this node has a blob */
        memcpy(p_data + size_left - sizeof(int), &b_node_has_blob, sizeof(int));
        size_left -= sizeof(int);

        /* Write the number of children */
        memcpy(p_data + size_left - sizeof(int), &n_initial_children, sizeof(int));
        size_left -= sizeof(int);
        /* Write the name of the node */
        memcpy(p_data + size_left - (sizeof(char) * MAX_NODENAME_LEN), p_node->p_name, sizeof(char) * MAX_NODENAME_LEN);
        size_left -= (sizeof(char) * MAX_NODENAME_LEN);
    }
    *p_size = size_left;
    return 0;
}   

int
blob_node_disassemble_data(blob_node **pp_node, unsigned char *p_data, size_t *p_size)
{
    size_t total_size = 0;
    blob_node *p_node = *pp_node;
    int b_has_blob;
    int child;
    if (p_node == NULL)
    {
        /* Only allocate a new node if there is not an existing one to repopulate */
        p_node = (blob_node*)calloc(sizeof(blob_node), 1);
        *pp_node = p_node;
        /* Assume blob structure unchanging across each blob tree - this does entire process does not
           need be re-executed for each new blob */
        memcpy(p_node->p_name, p_data + total_size, sizeof(char)*MAX_NODENAME_LEN);
        total_size += sizeof(char)*MAX_NODENAME_LEN;

        memcpy(&p_node->n_children, p_data + total_size, sizeof(int));
        total_size += sizeof(int);

        memcpy(&b_has_blob, p_data + total_size, sizeof(int));
        total_size += sizeof(int);

        if ((b_has_blob) && (NULL == p_node->p_blob))
        {
            blob_core_cfg cfg = {0};
            blob_core_init(&p_node->p_blob, &cfg);
        }
        
        if (b_has_blob)
        {
            size_t blob_size = 0;            
            blob_core_set_from_data(p_node->p_blob, p_data + total_size, &blob_size);
            total_size += blob_size;
        }
        for (child=0; child<p_node->n_children; child++)
        {
            size_t size_child;
            blob_node_disassemble_data(&p_node->ap_child_nodes[child], p_data + total_size, &size_child);
            p_node->ap_child_nodes[child]->p_parent_node = p_node;
            total_size += size_child;
        }
        p_node->blob_size = total_size;
    }
    else
    {
        /* Fast method for updating blob data w.r.t a new packet. Assumes consistent packet sizes */
        size_t blob_size = blob_core_get_serialized_data_size(p_node->p_blob);
        size_t node_header_size = sizeof(char)*MAX_NODENAME_LEN + 2 * sizeof(int);
        if (NULL != p_node->p_blob)
        {
            blob_core_update_root_data(p_node->p_blob, p_data + node_header_size);
        }
        
        for (child=0; child<p_node->n_children; child++)
        {
            size_t size_child;
            
            blob_node_disassemble_data(&p_node->ap_child_nodes[child], p_data + node_header_size + blob_size, &size_child);
        }
    }

    *p_size = p_node->blob_size;
    return 0;
}   
