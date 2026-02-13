#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "../include/blob.h"
#include "blob_node_tree.h"
#include "blob_core.h"
#include "blob_node.h"


#define IP_ADDRESS_BYTES 128
#define DOWNSTREAM_SERVER_HEADER_BYTES  (IP_ADDRESS_BYTES)

struct blob_node_tree_send_s
{
    blob_node *p_cur_node;
    int       (*p_send_cb)(void*, unsigned char*, size_t);
    void       *p_send_context;
};

struct blob_node_tree_retrieve_s
{
    blob_node       *p_root_node;
    blob_node       *p_cur_node;
    unsigned char   *p_data; /* Full data */
    size_t           n_data;
    int            (*p_rcv_cb)(void*, unsigned char**, size_t*);
    void            *p_rcv_context;
    int              b_new_data;
};

int
blob_node_tree_send_init(blob_node_tree_send **pp_nts, blob_nts_cfg *p_blob_nts_cfg)
{
    blob_node_tree_send* p_node_tree;
    p_node_tree = (blob_node_tree_send*)calloc(sizeof(blob_node_tree_send), 1);

    p_node_tree->p_send_cb = p_blob_nts_cfg->p_send_cb;
    p_node_tree->p_send_context = p_blob_nts_cfg->p_send_context;
    p_node_tree->p_cur_node = NULL;
    *pp_nts = p_node_tree;
    return BLOB_OK;
}

int
blob_node_tree_send_close(blob_node_tree_send **pp_nts)
{
    if (pp_nts == NULL || *pp_nts == NULL) return BLOB_OK;
    blob_node_tree_send *p_nts = *pp_nts;
    if (p_nts->p_cur_node)
    {
        // Go to root and close
        blob_node *p_node = p_nts->p_cur_node;
        while (p_node->p_parent_node) p_node = p_node->p_parent_node;
        blob_node_close(&p_node);
    }
    free(p_nts);
    *pp_nts = NULL;
    return BLOB_OK;
}

int
blob_node_tree_retrieve_init(blob_node_tree_retrieve **pp_nts, blob_ntr_cfg *p_blob_ntr_cfg)
{
    blob_node_tree_retrieve* p_node_tree;
    p_node_tree = (blob_node_tree_retrieve*)calloc(sizeof(blob_node_tree_retrieve), 1);

    p_node_tree->p_root_node = NULL;
    p_node_tree->p_cur_node = NULL;
    p_node_tree->p_data = NULL; /* Full data */
    p_node_tree->n_data = 0;
    p_node_tree->p_rcv_cb = p_blob_ntr_cfg->p_rcv_cb;
    p_node_tree->p_rcv_context = p_blob_ntr_cfg->p_rcv_context;
    p_node_tree->b_new_data = 0;

    *pp_nts = p_node_tree;
    return BLOB_OK;
}

int
blob_node_tree_retrieve_close(blob_node_tree_retrieve **pp_ntr)
{
    if (pp_ntr == NULL || *pp_ntr == NULL) return BLOB_OK;
    blob_node_tree_retrieve *p_ntr = *pp_ntr;
    if (p_ntr->p_root_node)
    {
        blob_node_close(&p_ntr->p_root_node);
    }
    // Note: p_ntr->p_data is NOT owned by this struct, it's owned by the comm layer callback
    free(p_ntr);
    *pp_ntr = NULL;
    return BLOB_OK;
}


int
blob_node_tree_send_start(blob_node_tree_send *p_nts, const char *node_name)
{
    blob_node *p_parent_temp;
    if (NULL == p_nts->p_cur_node)
    {
        p_nts->p_cur_node = (blob_node*)calloc(sizeof(blob_node), 1);
        strcpy(p_nts->p_cur_node->p_name, node_name);
    }
    else
    {   
        int b_found_node = 0;
        if (0 == strcmp(p_nts->p_cur_node->p_name, node_name))
        {
            b_found_node = 1;
        }
        for (int i=0; i<p_nts->p_cur_node->n_children; i++)
        {
            if (0 == strcmp(p_nts->p_cur_node->ap_child_nodes[i]->p_name, node_name))
            {
                p_parent_temp = p_nts->p_cur_node;
                p_nts->p_cur_node = p_nts->p_cur_node->ap_child_nodes[i];
                p_nts->p_cur_node->p_parent_node = p_parent_temp;
                b_found_node = 1;
            }
        }
        if (!b_found_node)
        {
            p_parent_temp = p_nts->p_cur_node;
            /* Create the node */
            p_nts->p_cur_node->ap_child_nodes[p_nts->p_cur_node->n_children] = (blob_node*)calloc(sizeof(blob_node), 1);
            
            /* Switch into the node */
            p_nts->p_cur_node = p_nts->p_cur_node->ap_child_nodes[p_nts->p_cur_node->n_children];
            p_nts->p_cur_node->p_parent_node = p_parent_temp;
            p_nts->p_cur_node->p_parent_node->n_children++;
            strcpy(p_nts->p_cur_node->p_name, node_name);
        }
    }
    return BLOB_OK;
}


int
blob_node_tree_send_flush(blob_node_tree_send *p_nts)
{
    if (NULL == p_nts->p_cur_node->p_parent_node)
    {
        size_t total_size;
        unsigned char *p_full_tree_blob;
        size_t total_size_copied;

        /* This is the root node */
        blob_node_aggregate_data(p_nts->p_cur_node, &total_size);
        p_full_tree_blob = (unsigned char*)calloc(sizeof(unsigned char), total_size);
        memset(p_full_tree_blob, 0, total_size);
    
        total_size_copied = total_size;
        /* Now, serialise the data */
        blob_node_assemble_data(p_nts->p_cur_node, p_full_tree_blob, &total_size_copied);

        /* All data be filled */
        assert(total_size_copied == 0);
        
        /* Send data via the provided send callback */
        p_nts->p_send_cb(p_nts->p_send_context, p_full_tree_blob, total_size);
        
        free(p_full_tree_blob);
        blob_node_close(&p_nts->p_cur_node);
    }
    else
    {
        size_t this_blob_size = 0;
        unsigned char *p_data;
        if (NULL != p_nts->p_cur_node)
        {
            blob_node_get_data(p_nts->p_cur_node, &p_data, &this_blob_size);
        }
        else
        {
            this_blob_size = 0;
        }
        
        p_nts->p_cur_node->blob_size = this_blob_size;
        p_nts->p_cur_node = p_nts->p_cur_node->p_parent_node;
    }
    
    return BLOB_OK;
}


int
blob_node_tree_retrieve_start(blob_node_tree_retrieve *p_ntr, const char *p_name)
{
    /* 1. If we are currently in a node, check if the requested node is one of its children */
    if (NULL != p_ntr->p_cur_node)
    {
        for (int i = 0; i < p_ntr->p_cur_node->n_children; i++)
        {
            if (0 == strcmp(p_name, p_ntr->p_cur_node->ap_child_nodes[i]->p_name))
            {
                p_ntr->p_cur_node = p_ntr->p_cur_node->ap_child_nodes[i];
                return 0;
            }
        }
    }

    /* 2. If it's not a child, check if it's a request for the root node (first time or reload) */
    if ( (NULL == p_ntr->p_root_node) || (0 == strcmp(p_name, p_ntr->p_root_node->p_name)) )
    {
        unsigned char *p_node_tree_data = NULL;
        size_t rcv_node_tree_bytes = 0;
        size_t total_size = 0;

        /* Flush/Clear existing root if we are reloading */
        if (NULL != p_ntr->p_root_node)
        {
            blob_node_close(&p_ntr->p_root_node);
        }

        /* Receive new data via the provided receive callback (e.g. jbuf pull) */
        p_ntr->p_rcv_cb(p_ntr->p_rcv_context, &p_ntr->p_data, &p_ntr->n_data);

        if (NULL == p_ntr->p_data)
        {
            return BLOB_ERR;
        }

        p_node_tree_data = p_ntr->p_data + DOWNSTREAM_SERVER_HEADER_BYTES;
        rcv_node_tree_bytes = p_ntr->n_data - DOWNSTREAM_SERVER_HEADER_BYTES;

        /* Disassemble the data and create the node-tree */
        blob_node_disassemble_data(&p_ntr->p_root_node, p_node_tree_data, &total_size);

        if (total_size != rcv_node_tree_bytes)
        {
            printf("Error decoding packet; size mismatch. Decode size %u, data size %u.\n", (unsigned int)total_size, (unsigned int)rcv_node_tree_bytes);
        }

        p_ntr->p_cur_node = p_ntr->p_root_node;
        p_ntr->b_new_data = 1;

        /* Verify the loaded root matches the requested name */
        if (0 != strcmp(p_name, p_ntr->p_root_node->p_name))
        {
            printf("Error, requested root node '%s' but received '%s'\n", p_name, p_ntr->p_root_node->p_name);
            return BLOB_ERR;
        }
        return 0;
    }

    printf("Error, invalid node name '%s'\n", p_name);
    return BLOB_ERR;
}

int
blob_node_tree_retrieve_flush(blob_node_tree_retrieve *p_ntr)
{
    if (NULL != p_ntr->p_cur_node)
    {
        if (NULL != p_ntr->p_cur_node->p_parent_node)
        {
            p_ntr->p_cur_node = p_ntr->p_cur_node->p_parent_node;
        }
        else
        {
            p_ntr->b_new_data = 0;
        }
    }
    return 0;
}

int
blob_node_tree_float_a(blob_node_tree_send *p_nts, const char *var_name, float *p_var_val, int n)
{
    return blob_node_float_a(p_nts->p_cur_node, var_name, p_var_val, n);
}

int
blob_node_tree_int_a(blob_node_tree_send *p_nts, const char *var_name, int *p_var_val, int n)
{
    return blob_node_int_a(p_nts->p_cur_node, var_name, p_var_val, n);
}

int
blob_node_tree_unsigned_int_a(blob_node_tree_send *p_nts, const char *var_name, unsigned int *p_var_val, int n)
{
    return blob_node_unsigned_int_a(p_nts->p_cur_node, var_name, p_var_val, n);
}

int
blob_node_tree_retrieve_float_a(blob_node_tree_retrieve *p_ntr, const char *var_name, const float **pp_var_val, int *p_n, int rep)
{
    *p_n = 0;
    *pp_var_val = NULL;
    if ((NULL != p_ntr->p_data) && (NULL != p_ntr->p_cur_node))
    {
        blob_node_retrieve_float_a(p_ntr->p_cur_node, var_name, pp_var_val, p_n, rep);
    }
    return 0;
}

int
blob_node_tree_retrieve_int_a(blob_node_tree_retrieve *p_ntr, const char *var_name, const int **pp_var_val, int *p_n, int rep)
{
    *p_n = 0;
    *pp_var_val = NULL;
    if ((NULL != p_ntr->p_data) && (NULL != p_ntr->p_cur_node))
    {
        blob_node_retrieve_int_a(p_ntr->p_cur_node, var_name, pp_var_val, p_n, rep);
    }
    return 0;
}

int
blob_node_tree_retrieve_unsigned_int_a(blob_node_tree_retrieve *p_ntr, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep)
{
    *p_n = 0;
    *pp_var_val = NULL;
    if ((NULL != p_ntr->p_data) && (NULL != p_ntr->p_cur_node))
    {
        blob_node_retrieve_unsigned_int_a(p_ntr->p_cur_node, var_name, pp_var_val, p_n, rep);
    }
    return 0;
}
