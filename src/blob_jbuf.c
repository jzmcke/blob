#include <stdio.h>
#include <stdlib.h>

#include "blob_jbuf.h"

struct blob_jbuf_s
{
    void    **pp_buffer; /* blob_jbuf can buffer any data type */
    size_t   *p_size;
    int       push_idx;
    int       pull_idx;
    int       jbuf_len;
};

int
blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg)
{
    blob_jbuf *p_jbuf;
    p_jbuf = (blob_jbuf*)calloc(sizeof(blob_jbuf), 1);
    p_jbuf->pp_buffer = (void**)calloc(sizeof(void*), p_jbuf_cfg->jbuf_len);
    p_jbuf->p_size = (size_t*)calloc(sizeof(size_t), p_jbuf_cfg->jbuf_len);
    if (p_jbuf_cfg->jbuf_len < 2)
    {
        blob_jbuf_close(&p_jbuf);
        printf("Error, jbuf_len must be greater than 2.\n");
        return BLOB_JBUF_ERR;
    }
    p_jbuf->jbuf_len = p_jbuf_cfg->jbuf_len;
    for (int i=0; i<p_jbuf_cfg->jbuf_len; i++)
    {
        p_jbuf->pp_buffer[i] = NULL;
        p_jbuf->p_size[i] = 0;
    }
    p_jbuf->push_idx = 0;
    p_jbuf->pull_idx = 0;
    *pp_jbuf = p_jbuf;
    return 0;
}

/* Push an element onto the buffer. Push index will never be incremented over the pull.
   Anything that makes it into the queue stays in the queue until it is pulled by the caller
   process and freed. blob_jbuf will never free an object already in the queue itself, but
   will drop packets before it has been added to the queue. */
int
blob_jbuf_push(blob_jbuf *p_jbuf, void *p_new_data, size_t n)
{
    int ret = BLOB_JBUF_OK;
    /* The pull process may be processing this frame now, so don't write over the top of it. */
    if ((NULL == p_jbuf->pp_buffer[p_jbuf->push_idx]) && (p_jbuf->push_idx != p_jbuf->pull_idx))
    {
        /* The queue has space for write */
        p_jbuf->pp_buffer[p_jbuf->push_idx] = p_new_data;
        p_jbuf->p_size[p_jbuf->push_idx] = n;
        p_jbuf->push_idx = (p_jbuf->push_idx + 1) % p_jbuf->jbuf_len;
    }
    else if ((NULL != p_jbuf->pp_buffer[p_jbuf->push_idx]) && (p_jbuf->push_idx != p_jbuf->pull_idx))
    {
        /* This condition should never arise, because the push index should never increment past the pull index. */
        printf("Error, something bad has happened in blob_jbuf logic!\n");
        ret = BLOB_JBUF_ERR;
    }
    else
    {
        /* queue is full, drop the current packet. */
        free(p_new_data);
        ret = BLOB_JBUF_DROPPED_PACKET;
    }
    return ret;
}


/* Pull an element from the buffer */
int
blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_new_data, size_t *p_n)
{
    *pp_new_data = p_jbuf->pp_buffer[p_jbuf->pull_idx];
    *p_n = p_jbuf->p_size[p_jbuf->pull_idx];
    p_jbuf->pull_idx = (p_jbuf->pull_idx + 1) % p_jbuf->jbuf_len;
}

int
blob_jbuf_close(blob_jbuf **pp_jbuf)
{
    for (int i=0; i<(*pp_jbuf)->jbuf_len; i++)
    {
        if (NULL != (*pp_jbuf)->pp_buffer)
        {
            free((*pp_jbuf)->pp_buffer[i]);
        }
    }
    free((*pp_jbuf)->pp_buffer);
    free((*pp_jbuf)->p_size);
    free(*pp_jbuf);
}
