#include "packet.h"
#include <string.h>
#include <stdlib.h>


int
packet_init(packet *p_packet, packet_cfg *p_cfg)
{
    p_packet->p_fragments = NULL;
    p_packet->total_fragments = p_cfg->total_fragments;
    p_packet->received_fragments = 0;
    p_packet->p_unfragmented_data = NULL;
    p_packet->unfragmented_size = 0;
    p_packet->deallocate_callback = p_cfg->deallocate_callback;
    p_packet->p_deallocate_context = p_cfg->p_context;
    p_packet->seq_num = -1;

    p_packet->p_fragments = (fragment*)malloc(p_packet->total_fragments * sizeof(fragment));
    for (int i=0; i<p_packet->total_fragments; i++)
    {
        p_packet->p_fragments[i].p_fragment_data = NULL;
        p_packet->p_fragments[i].size = 0;
        p_packet->p_fragments[i].b_occupied = 0;
    }
    return PACKET_OK;
}

int
packet_close(packet *p_packet)
{
    packet_deep_empty(p_packet);
    free(p_packet);
    return PACKET_OK;
}

int
packet_deep_empty(packet *p_packet)
{
    if (p_packet->p_fragments != NULL)
    {
        free(p_packet->p_fragments);
        p_packet->p_fragments = NULL;
    }
    if (p_packet->p_unfragmented_data != NULL)
    {
        free(p_packet->p_unfragmented_data);
        p_packet->p_unfragmented_data = NULL;
    }
    p_packet->total_fragments = 0;
    return PACKET_OK;
}

int
packet_shallow_empty(packet *p_packet)
{
    for (int i=0; i<p_packet->total_fragments; i++)
    {
        p_packet->p_fragments[i].b_occupied = 0;
    }
    p_packet->received_fragments = 0;
    p_packet->seq_num = -1;
    return PACKET_OK;
}


int
packet_reset(packet *p_packet, packet_cfg *p_cfg)
{
    if (p_packet->total_fragments != p_cfg->total_fragments)
    {
        packet_deep_empty(p_packet);
        
        packet_init(p_packet, p_cfg);
    }
    else
    {
        // Doesn't deallocate. Just resets the packet.
        packet_shallow_empty(p_packet);
    }
    return PACKET_OK;
}

int
packet_is_fragment_empty(packet *p_packet, int frag_idx)
{
    return p_packet->p_fragments[frag_idx].b_occupied == 0;
}

int
packet_add_fragment_data(packet *p_packet, unsigned char *p_data, size_t n_data)
{
    int frag_idx = *((int*)p_data + 1);

    p_packet->received_fragments++;
    // Allocate space for the fragment if it does not already exist, otherwise just copy the data
    if (p_packet->p_unfragmented_data == NULL)
    {
        if (p_packet->p_fragments[frag_idx].p_fragment_data != NULL)
        {
            // This condition will arise if a packet is received twice. In this case, we need to free the old data.
            if (NULL != p_packet->deallocate_callback)
            {
                p_packet->deallocate_callback(p_packet->p_fragments[frag_idx].p_fragment_data - 3 * sizeof(int), p_packet->p_deallocate_context);
            }
        }
        p_packet->p_fragments[frag_idx].p_fragment_data = (unsigned char *)((int*)p_data + 3);
        p_packet->p_fragments[frag_idx].size = n_data - 3 * sizeof(int); // The first 3 elements are seq_num, frag_idx, and total_frags

        // We have received all the packets, so we can finally allocate space for the unfragmented data
        if (p_packet->received_fragments == p_packet->total_fragments)
        {
            size_t i_data = 0;
            p_packet->unfragmented_size = 0;
            for (int i=0; i<p_packet->total_fragments; i++)
            {
                p_packet->unfragmented_size += p_packet->p_fragments[i].size;
            }
            p_packet->p_unfragmented_data = (unsigned char*)malloc(p_packet->unfragmented_size);
            for (int i=0; i<p_packet->total_fragments; i++)
            {
                /* Copy the fragmented data onto the unfragmented memory in the correct position. */
                memcpy(p_packet->p_unfragmented_data + i_data, p_packet->p_fragments[i].p_fragment_data, p_packet->p_fragments[i].size);
                if (NULL != p_packet->deallocate_callback)
                {
                    p_packet->deallocate_callback(p_packet->p_fragments[i].p_fragment_data - 3 * sizeof(int), p_packet->p_deallocate_context);
                }
                /* For future frames, the memory has already been assigned. */
                p_packet->p_fragments[i].p_fragment_data = p_packet->p_unfragmented_data + i_data;

                i_data += p_packet->p_fragments[i].size;
            }
        }
    }
    else
    {
        memcpy(p_packet->p_fragments[frag_idx].p_fragment_data, (unsigned char *)((int*)p_data + 3), n_data - 3 * sizeof(int));
        if (NULL != p_packet->deallocate_callback)
        {
            p_packet->deallocate_callback(p_data, p_packet->p_deallocate_context);
        }
    }

    p_packet->p_fragments[frag_idx].b_occupied = 1;
    p_packet->p_fragments[frag_idx].size = n_data - 3 * sizeof(int);

    return PACKET_OK;
}

int
packet_is_full(packet *p_packet)
{
    return p_packet->received_fragments == p_packet->total_fragments;
}

int
packet_set_seq_num(packet *p_packet, int seq_num)
{
    p_packet->seq_num = seq_num;
    return PACKET_OK;
}
