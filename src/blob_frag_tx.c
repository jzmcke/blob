#include "blob_frag_tx.h"
#include <stddef.h>
#include <stdlib.h>

struct blob_frag_tx_s
{
    unsigned char *p_data;
    size_t data_size;
    size_t n_remaining;
    size_t n_total_written;
    size_t frag_size;
    int frag_idx;
    int seq_num;
    int n_frags;
    unsigned char *p_out_buffer;
};

int
set_fragment_header(blob_frag_tx *p_blob_frag_tx, int seq_num, int frag_idx, int total_frags)
{
    unsigned char *p_send_data = p_blob_frag_tx->p_out_buffer;
    *((int*)p_send_data) = seq_num;
    *((int*)p_send_data + 1) = frag_idx;
    *((int*)p_send_data + 2) = total_frags;
    return 0;
}

int
set_fragment_data(blob_frag_tx *p_blob_frag_tx, unsigned char *p_data, size_t data_size)
{
    unsigned char *p_send_data = p_blob_frag_tx->p_out_buffer;
    memcpy(p_send_data + 3 * sizeof(int), p_data, data_size);
    return 0;
}


int
blob_frag_tx_init(blob_frag_tx **pp_blob_frag_tx, size_t frag_size)
{
    blob_frag_tx *p_blob_frag_tx = (blob_frag_tx*)calloc(1, sizeof(blob_frag_tx));
    p_blob_frag_tx->frag_size = frag_size;
    // Allocate memory for the output buffer + the packet header: seq num, frag idx, n fragss
    p_blob_frag_tx->p_out_buffer = (unsigned char*)malloc(frag_size + 3 *sizeof(int));
    p_blob_frag_tx->seq_num = 0;

    *pp_blob_frag_tx = p_blob_frag_tx;
    
    return 0;
}

int
blob_frag_tx_begin_packet(blob_frag_tx *p_blob_frag_tx,
                          unsigned char *p_data,
                          size_t n)
{
    p_blob_frag_tx->n_frags = (int)(n / p_blob_frag_tx->frag_size) + 1;
    p_blob_frag_tx->p_data = p_data;
    p_blob_frag_tx->data_size = n;
    p_blob_frag_tx->frag_idx = 0;
    p_blob_frag_tx->n_remaining = n;
    p_blob_frag_tx->n_total_written = 0;
    return 0;
}


int
blob_frag_tx_next_packet(blob_frag_tx *p_blob_frag_tx, unsigned char **pp_data, size_t *p_n)
{
    size_t n_write = 0;
    if (p_blob_frag_tx->n_remaining > 0)
    {
        n_write = p_blob_frag_tx->n_remaining > p_blob_frag_tx->frag_size ? p_blob_frag_tx->frag_size : p_blob_frag_tx->n_remaining;
        {
            set_fragment_header(p_blob_frag_tx, p_blob_frag_tx->seq_num, p_blob_frag_tx->frag_idx, p_blob_frag_tx->n_frags);
            set_fragment_data(p_blob_frag_tx, p_blob_frag_tx->p_data + p_blob_frag_tx->n_total_written, n_write);
        }
        p_blob_frag_tx->n_remaining = p_blob_frag_tx->n_remaining - n_write;
        p_blob_frag_tx->n_total_written = p_blob_frag_tx->data_size - p_blob_frag_tx->n_remaining;
        p_blob_frag_tx->frag_idx++;
        *pp_data = p_blob_frag_tx->p_out_buffer;
        *p_n = n_write + 3 * sizeof(int);
    }
    else
    {
        p_blob_frag_tx->seq_num++;
        p_blob_frag_tx->frag_idx = 0;
        p_blob_frag_tx->n_remaining = p_blob_frag_tx->data_size;
        p_blob_frag_tx->n_total_written = 0;
        *pp_data = NULL;
        *p_n = 0;
    }

    return 0;
}