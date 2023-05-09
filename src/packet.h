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
    void (*deallocate_callback)(unsigned char *p_data, void *p_context);
    void *p_deallocate_context;
} packet;

typedef struct packet_cfg_s
{
    int total_fragments;
    void (*deallocate_callback)(unsigned char *p_data, void *p_context);
    void *p_context;
} packet_cfg;


int
packet_init(packet *p_packet, packet_cfg *p_cfg);

int
packet_close(packet *p_packet);

int
packet_deep_empty(packet *p_packet);

int
packet_shallow_empty(packet *p_packet);

int
packet_reset(packet *p_packet, packet_cfg *p_cfg);

int
packet_is_fragment_empty(packet *p_packet, int frag_idx);

int
packet_add_fragment_data(packet *p_packet, unsigned char *p_data, size_t n_data);

int
packet_is_full(packet *p_packet);

int
packet_set_seq_num(packet *p_packet, int seq_num);


