#include <stddef.h>

#define PACKET_OK 0
#define PACKET_ERR (-1)

typedef struct fragment_s
{
    unsigned char *p_fragment_data;
    int b_occupied;
    size_t size;
} fragment;

typedef struct packet_s
{
    fragment *p_fragments;
    int total_fragments;
    int received_fragments;
    unsigned char *p_unfragmented_data;
    size_t unfragmented_size;
    int seq_num;
} packet;


int
packet_init(packet *p_packet, int total_fragments);

int
packet_close(packet *p_packet);

int
packet_deep_empty(packet *p_packet);

int
packet_shallow_empty(packet *p_packet);

int
packet_reset(packet *p_packet, int total_fragments);

int
packet_is_fragment_empty(packet *p_packet, int frag_idx);

int
packet_add_fragment_data(packet *p_packet, unsigned char *p_data, size_t n_data, int frag_idx);

int
packet_is_full(packet *p_packet);

int
packet_set_seq_num(packet *p_packet, int seq_num);


