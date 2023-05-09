#include <stdio.h>
#include <stdlib.h>

#include "blob_jbuf_frag.h"
#include "packet.h"


struct blob_jbuf_s
{
    packet *p_packets; /* blob_jbuf can buffer any data type */
    int          push_idx;
    int          pull_idx;
    int          jbuf_len;
    int          buffer_fullness;
    int          latency;
    int          b_exit_emptiness;
    int          b_exit_fullness;
    packet_cfg   packet_cfg;
};

int
blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg)
{
    blob_jbuf *p_jbuf;
    p_jbuf = (blob_jbuf*)calloc(1, sizeof(blob_jbuf));
    p_jbuf->p_packets = (packet*)calloc(p_jbuf_cfg->jbuf_len, sizeof(packet));
    p_jbuf->buffer_fullness = 0;
    p_jbuf->b_exit_emptiness = 1;
    p_jbuf->b_exit_fullness = 0;
    p_jbuf->jbuf_len = p_jbuf_cfg->jbuf_len;
    p_jbuf->latency = p_jbuf->jbuf_len / 2;

    p_jbuf->packet_cfg.deallocate_callback = p_jbuf_cfg->deallocate_callback;
    p_jbuf->packet_cfg.p_context = p_jbuf_cfg->p_context;
    p_jbuf->packet_cfg.total_fragments = 1;
    if (p_jbuf_cfg->jbuf_len < 2)
    {
        blob_jbuf_close(&p_jbuf);
        printf("Error, jbuf_len must be greater than 2.\n");
        return BLOB_JBUF_ERR;
    }

    for (int i=0; i<p_jbuf_cfg->jbuf_len; i++)
    {
        packet_init(&p_jbuf->p_packets[i], &p_jbuf->packet_cfg);
    }
    p_jbuf->push_idx = 0;
    p_jbuf->pull_idx = p_jbuf->jbuf_len - 1;
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
    int *p_new_data_ints = (int *)p_new_data;

    // Could add these as input arguments to the function itself.
    int seq_num = p_new_data_ints[0];
    int frag_idx = p_new_data_ints[1];
    int total_fragments = p_new_data_ints[2];

    unsigned char *p_fragment_data = (unsigned char *)&p_new_data_ints[3];
    packet *p_packet = NULL;

    // Push the new fragment into the buffer queue
    p_jbuf->push_idx = seq_num % p_jbuf->jbuf_len;

    p_packet = &p_jbuf->p_packets[p_jbuf->push_idx];

    // This line need to be here, since the number of fragments is not known until the first fragment is received.
    if (p_packet->total_fragments != total_fragments)
    {
        p_jbuf->packet_cfg.total_fragments = total_fragments;
        packet_reset(p_packet, &p_jbuf->packet_cfg);
    }

    if (packet_is_full(p_packet))
    {
        p_jbuf->packet_cfg.total_fragments = total_fragments;
        packet_reset(p_packet, &p_jbuf->packet_cfg);
        p_jbuf->buffer_fullness--;
        ret = BLOB_JBUF_DROPPED_PACKET;
    }
    // This < operation will fall over if the sequence number wraps around, can make more robust by using a modulo operation.
    // This stops older packets from overwriting newer packets (a != would not suffice)
    if (  (p_packet->seq_num != -1)
        &&(p_packet->seq_num  < seq_num))
    {
        p_jbuf->packet_cfg.total_fragments = total_fragments;
        packet_reset(p_packet, &p_jbuf->packet_cfg);
        printf("Overwrote packet at idx %d!\n", p_jbuf->push_idx);
        ret = BLOB_JBUF_OVERWROTE_PACKET;
    }

    if (p_packet->seq_num > seq_num)
    {
        printf("Dropped packet at idx %d!\n", p_jbuf->push_idx);
        return BLOB_JBUF_DROPPED_PACKET;
    }

    packet_set_seq_num(p_packet, seq_num);
    
    /* Only add a fragment if the buffer is not attempting latency recovery, the sequence numbers are a match, and there is no data already in the buffer.*/
    if (   (packet_is_fragment_empty(p_packet, frag_idx))
        && (!p_jbuf->b_exit_fullness))
    {
        packet_add_fragment_data(p_packet, p_new_data, n);
        
    }
    else
    {
        printf("Dropped fragment at idx %d!\n", p_jbuf->push_idx);
        ret = BLOB_JBUF_DROPPED_FRAGMENT;
    }
    
    if (packet_is_full(p_packet))
    {
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
        // Return JBUF_FRAGMENT_RECEIVED only if there as been no error so far.
        ret = ret == BLOB_JBUF_OK ? BLOB_JBUF_FRAGMENT_RECEIVED : ret;
    }
    
    return ret;
}


/* Pull an element from the buffer */
int
blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_new_data, size_t *p_n)
{
    int ret = BLOB_JBUF_OK;
    /* This function gets called only once we are confident the latest packet isnt required anymore!
    Assumes that receiver has already pulled the latest packet and used it. */
    *pp_new_data = NULL;
    *p_n = 0;
    if (!p_jbuf->b_exit_emptiness)
    {
        /* This condition is true if the buffer has sufficient packets.
        if the buffer underflows, then b_exit_emptiness should be set false until the buffer
        has reached the latency setting */

        if (packet_is_full(&p_jbuf->p_packets[p_jbuf->pull_idx]))
        {
            p_jbuf->buffer_fullness--;
            // the buffer fullness is only incremented when a full packet has been received.
        }
        // Start by freeing the last packet on the buffer.
        p_jbuf->packet_cfg.total_fragments =  p_jbuf->p_packets[p_jbuf->pull_idx].total_fragments;
        packet_reset(&p_jbuf->p_packets[p_jbuf->pull_idx], &p_jbuf->packet_cfg);
        p_jbuf->pull_idx = (p_jbuf->pull_idx + 1) % p_jbuf->jbuf_len;        

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
        if (packet_is_full(&p_jbuf->p_packets[p_jbuf->pull_idx]))
        {
            *pp_new_data = p_jbuf->p_packets[p_jbuf->pull_idx].p_unfragmented_data;
            *p_n = p_jbuf->p_packets[p_jbuf->pull_idx].unfragmented_size;
        }
    }
    
    return ret;
}


int
blob_jbuf_close(blob_jbuf **pp_jbuf)
{
    for (int i=0; i<(*pp_jbuf)->jbuf_len; i++)
    {
        if (NULL != &((*pp_jbuf)->p_packets[i]))
        {
            packet_close(&((*pp_jbuf)->p_packets[i]));
        }
    }
    free((*pp_jbuf)->p_packets);
    free(*pp_jbuf);
    return BLOB_JBUF_OK;
}
