/* Responsible for send data callback, populating a shared receive data array and jitter buffering the network traffic */
#ifdef BLOB_ESP32_UDP
#include "AsyncUDP.h"
#include "../include/blob.h"
#include "blob_comm.h"

#include <memory>
#include <iostream>


extern "C"
{
#include "blob_jbuf_frag.h"
}

#define MTU_SIZE 1400 // This is a requirement of the ESP32 UDP stack


int
_blob_espudp_init(blob_comm_cfg *p_cfg, int addr0, int addr1, int addr2, int addr3, int port, int n_latency);

int
_blob_espudp_terminate(blob_comm_cfg *p_blob_comm_cfg);

int
_blob_espudp_rcv_callback(void *p_context, unsigned char **pp_recv_data, size_t *p_recv_total_size);

int
_blob_espudp_send_callback(void *p_context, unsigned char *p_send_data, size_t total_size);

typedef struct blob_espudp_state_s
{
    AsyncUDP *p_udp_client;
    blob_jbuf *p_blob_jbuf;
    unsigned char *p_processed_data;
    size_t n_data;
    IPAddress dest_ip_addr;
    int dest_port;
    size_t max_packet_tx;
    unsigned char *p_send_data;
    int seq_num;

} blob_espudp_state;

int
set_fragment_header(unsigned char *p_send_data, int seq_num, int frag_idx, int total_frags)
{
    *((int*)p_send_data) = seq_num;
    *((int*)p_send_data + 1) = frag_idx;
    *((int*)p_send_data + 2) = total_frags;
    return 0;
}

int
set_fragment_data(unsigned char *p_send_data, unsigned char *p_data, size_t data_size)
{
    memcpy(p_send_data + 3 * sizeof(int), p_data, data_size);
    return 0;
}

int
_blob_espudp_init(blob_comm_cfg *p_cfg, int serv_addr0, int serv_addr1, int serv_addr2, int serv_addr3, int port, int n_buf)
{
    blob_espudp_state *p_espudp;
    blob_jbuf_cfg jbuf_cfg;
    p_espudp = (blob_espudp_state*)calloc(sizeof(blob_espudp_state), 1);
    p_espudp->p_udp_client = new AsyncUDP();
    p_espudp->dest_ip_addr = IPAddress(serv_addr0, serv_addr1, serv_addr2, serv_addr3);
    p_espudp->dest_port = port;
    p_espudp->max_packet_tx = MTU_SIZE;
    p_espudp->p_send_data = (unsigned char*)calloc(sizeof(unsigned char), p_espudp->max_packet_tx);
    p_espudp->seq_num = 0;

    jbuf_cfg.jbuf_len = n_buf;
    blob_jbuf_init(&p_espudp->p_blob_jbuf, &jbuf_cfg);
    p_espudp->p_udp_client->listen(4567);
    p_espudp->p_udp_client->onPacket([p_espudp](AsyncUDPPacket packet)
        {
            unsigned char *p_data;
            p_data = (unsigned char*)calloc(sizeof(unsigned char), packet.length());
            memcpy(p_data, packet.data(), packet.length());
            blob_jbuf_push(p_espudp->p_blob_jbuf, (void*)p_data, packet.length());
        });
    if (p_espudp->p_udp_client->connect(p_espudp->dest_ip_addr, p_espudp->dest_port))
    {
        printf("UDP connection to server established!\n");
        /* Truly asynchronous, so can be executed whenever. Memory population needs to be an atomic operation. */    
    }
    else
    {
        printf("UDP connection to server failed!\n");
    }
    p_cfg->p_send_cb = _blob_espudp_send_callback;
    p_cfg->p_send_context = (void*)p_espudp;
    p_cfg->p_rcv_cb = _blob_espudp_rcv_callback;
    p_cfg->p_rcv_context = (void*)p_espudp;
    return 0;
}

int
_blob_espudp_terminate(blob_comm_cfg *p_blob_comm_cfg)
{
    esp_err_t err;
    blob_espudp_state *p_espudp = (blob_espudp_state*)p_blob_comm_cfg->p_send_context;
    /* Should probably implement this for a reason i'm not sure about */
    return 0;
}

int
_blob_espudp_send_callback(void *p_context, unsigned char *p_send_data, size_t total_size)
{
    blob_espudp_state *p_espudp = (blob_espudp_state*)p_context;
    size_t n_write = 0;
    size_t n_sent;
    size_t n_remaining = total_size;
    size_t n_total_written = 0;
    int frag_idx = 0;
    int total_frags = total_size / p_espudp->max_packet_tx + 1;
    while (n_remaining > 0)
    {
        n_write = n_remaining > p_espudp->max_packet_tx ? p_espudp->max_packet_tx : n_remaining;
        {
            std::unique_ptr<AsyncUDPMessage> wt_pkt(new AsyncUDPMessage(n_write));
            set_fragment_header(p_espudp->p_send_data, p_espudp->seq_num, frag_idx, total_frags);
            set_fragment_data(p_espudp->p_send_data, p_send_data + n_total_written, n_write);
            wt_pkt->write(p_espudp->p_send_data, n_write);
            n_sent = p_espudp->p_udp_client->sendTo(*wt_pkt, p_espudp->dest_ip_addr, p_espudp->dest_port, TCPIP_ADAPTER_IF_MAX);
        }

        if (n_write != n_sent)
        {
            printf("Error transmitting packet. Total size attempted to transmit %d, actual size transmitted %d\n", total_size, n_write);
        }
        n_remaining = n_remaining - n_write;
        n_total_written = total_size - n_remaining;
        frag_idx++;
    }
    p_espudp->seq_num++;
    return 0;
};

int
_blob_espudp_rcv_callback(void *p_context, unsigned char **pp_recv_data, size_t *p_recv_total_size)
{
    blob_espudp_state *p_state = (blob_espudp_state*)p_context;
    if (*p_recv_total_size > 0)
    {
        // printf("Total received size %u\n", (unsigned int)*p_recv_total_size);
    }
    blob_jbuf_pull(p_state->p_blob_jbuf, (void**)&p_state->p_processed_data, &p_state->n_data);
    if (p_state->p_processed_data != NULL)
    {
        // printf("Received data size %u\n", (unsigned int)p_state->n_data);
    }
    *pp_recv_data = p_state->p_processed_data;
    *p_recv_total_size = p_state->n_data;
    return 0;
};

#endif