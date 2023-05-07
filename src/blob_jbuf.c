
#if 0
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
    int       buffer_fullness;
    int       latency;
    int       b_exit_emptiness;
    int       b_exit_fullness;
};

int
blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg)
{
    blob_jbuf *p_jbuf;
    p_jbuf = (blob_jbuf*)calloc(sizeof(blob_jbuf), 1);
    p_jbuf->pp_buffer = (void**)calloc(sizeof(void*), p_jbuf_cfg->jbuf_len);
    p_jbuf->p_size = (size_t*)calloc(sizeof(size_t), p_jbuf_cfg->jbuf_len);
    p_jbuf->buffer_fullness = 0;
    p_jbuf->b_exit_emptiness = 1;
    p_jbuf->b_exit_fullness = 0;
    p_jbuf->jbuf_len = p_jbuf_cfg->jbuf_len;
    p_jbuf->latency = p_jbuf->jbuf_len / 2;
    if (p_jbuf_cfg->jbuf_len < 2)
    {
        blob_jbuf_close(&p_jbuf);
        printf("Error, jbuf_len must be greater than 2.\n");
        return BLOB_JBUF_ERR;
    }
   
    for (int i=0; i<p_jbuf_cfg->jbuf_len; i++)
    {
        p_jbuf->pp_buffer[i] = NULL;
        p_jbuf->p_size[i] = 0;
    }
    p_jbuf->push_idx = 0;
    p_jbuf->pull_idx = p_jbuf->push_idx;
    printf("jbuf_len: %d\n", p_jbuf->jbuf_len);
    printf("jbuf_latency: %d\n", p_jbuf->latency);
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
    /* Only write to the buffer if it is NULL (and the pull process has freed it) */
    if (  (!p_jbuf->b_exit_fullness)
        &&(NULL == p_jbuf->pp_buffer[p_jbuf->push_idx]))
    {
        /* The queue has space for write */
        p_jbuf->pp_buffer[p_jbuf->push_idx] = p_new_data;
        p_jbuf->p_size[p_jbuf->push_idx] = n;
        p_jbuf->push_idx = (p_jbuf->push_idx + 1) % p_jbuf->jbuf_len;
        p_jbuf->buffer_fullness++;
        if (p_jbuf->buffer_fullness >= p_jbuf->latency)
        {
            p_jbuf->b_exit_emptiness = 0;
        }
        if (p_jbuf->buffer_fullness > p_jbuf->jbuf_len)
        {
            /* This condition should never arise, because we should never push when the buffer is already full. */     
            ret = BLOB_JBUF_ERR;
        }
    }
    else
    {
        /* This condition should never arise, because the push index should never increment past the pull index. */
        // printf("Error, something bad has happened in blob_jbuf logic!\n");
        /* queue is full, drop the current packet. */
        free(p_new_data);
        p_jbuf->b_exit_fullness = 1;
        printf("Error, attempted push when buffer already full!\n");
        printf("Dropped packet at idx %d!\n", p_jbuf->push_idx);
        ret = BLOB_JBUF_DROPPED_PACKET;
        // ret = BLOB_JBUF_ERR;
    }
    return ret;
}


/* Pull an element from the buffer */
int
blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_new_data, size_t *p_n)
{
    *pp_new_data = p_jbuf->pp_buffer[p_jbuf->pull_idx];
    *p_n = p_jbuf->p_size[p_jbuf->pull_idx];
    /* This increment is done inside blob_jbuf_release_latest_entry, which means blob_jbuf_release_latest_entry should 
    be called in the same loop as blob_jbuf_pull */
    // p_jbuf->pull_idx = (p_jbuf->pull_idx + 1) % p_jbuf->jbuf_len;
}

int
blob_jbuf_release_latest_entry(blob_jbuf *p_jbuf)
{
    /* This function gets called only once we are confident the latest packet isnt required anymore!
       Assumes that receiver has already pulled the latest packet and used it. */
    int ret = BLOB_JBUF_OK;
    if (  (!p_jbuf->b_exit_emptiness)
        &&(NULL != p_jbuf->pp_buffer[p_jbuf->pull_idx])
        )
    {
        /* This condition is true if the buffer has sufficient packets.
           if the buffer underflows, then b_exit_emptiness should be set false until the buffer
           has reached the latency setting */
        free(p_jbuf->pp_buffer[p_jbuf->pull_idx]);
        p_jbuf->pp_buffer[p_jbuf->pull_idx] = NULL;
        p_jbuf->p_size[p_jbuf->pull_idx] = 0;
        p_jbuf->pull_idx = (p_jbuf->pull_idx + 1) % p_jbuf->jbuf_len;
        p_jbuf->buffer_fullness--;
        if (p_jbuf->buffer_fullness == 0)
        {
            /* We need to wait until enough packets have been pushed onto the buffer to resume playback*/
            p_jbuf->b_exit_emptiness = 1;
        }
        if (p_jbuf->buffer_fullness <= p_jbuf->latency)
        {
            p_jbuf->b_exit_fullness = 0;
        }

        if (p_jbuf->buffer_fullness < 0)
        {
            /* This condition should never arise, because we should never pull from the buffer when the buffer is empty. */
            printf("Error, attempted pull when buffer already empty!\n");
            ret = BLOB_JBUF_ERR;
        }
    }
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
#endif