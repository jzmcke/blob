#include "packet.h"
#include <string.h>
#include <stdlib.h>

int
packet_init(packet *p_packet, int total_fragments)
{
    p_packet->p_fragments = NULL;
    p_packet->total_fragments = total_fragments;
    p_packet->received_fragments = 0;
    p_packet->p_unfragmented_data = NULL;
    p_packet->unfragmented_size = 0;
    p_packet->seq_num = -1;

    p_packet->p_fragments = (fragment*)malloc(total_fragments * sizeof(fragment));
    for (int i=0; i<total_fragments; i++)
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
packet_reset(packet *p_packet, int total_fragments)
{
    if (p_packet->total_fragments != total_fragments)
    {
        packet_deep_empty(p_packet);
        packet_init(p_packet, total_fragments);
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
packet_add_fragment_data(packet *p_packet, unsigned char *p_data, size_t n_data, int frag_idx)
{
    // Allocate space for the fragment if it does not already exist, otherwise just copy the data
    if (p_packet->p_fragments[frag_idx].p_fragment_data == NULL)
    {
        size_t prev_size = p_packet->unfragmented_size;
        size_t i_data = 0;
        unsigned char *p_new_unfragmented_data = NULL;

        p_new_unfragmented_data = (unsigned char*)malloc(prev_size + n_data);
        p_packet->unfragmented_size = prev_size + n_data;

        for (int i=0; i<p_packet->total_fragments; i++)
        {
            memcpy(p_new_unfragmented_data + n_data, p_packet->p_fragments[i].p_fragment_data, p_packet->p_fragments[i].size);
            i_data += p_packet->p_fragments[i].size;
        }
        free(p_packet->p_unfragmented_data);
        p_packet->p_unfragmented_data = p_new_unfragmented_data;
        p_packet->p_fragments[frag_idx].p_fragment_data = p_packet->p_unfragmented_data + i_data;
    }
    
    memcpy(p_packet->p_fragments[frag_idx].p_fragment_data, p_data, n_data);
    p_packet->p_fragments[frag_idx].b_occupied = 1;
    p_packet->p_fragments[frag_idx].size = n_data;
    p_packet->received_fragments++;
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
