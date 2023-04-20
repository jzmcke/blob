/* Responsible for send data callback, populating a shared receive data array and jitter buffering the network traffic */
#ifdef BLOB_ESP32_UDP
#include "AsyncUDP.h"
#include "../include/blob.h"
#include "blob_comm.h"

#include <memory>
#include <iostream>

extern "C"
{
#include "blob_jbuf.h"
}


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
} blob_espudp_state;

int
_blob_espudp_init(blob_comm_cfg *p_cfg, int serv_addr0, int serv_addr1, int serv_addr2, int serv_addr3, int port, int n_buf)
{
    blob_espudp_state *p_espudp;
    blob_jbuf_cfg jbuf_cfg;
    p_espudp = (blob_espudp_state*)calloc(sizeof(blob_espudp_state), 1);
    p_espudp->p_udp_client = new AsyncUDP();
    p_espudp->dest_ip_addr = IPAddress(serv_addr0, serv_addr1, serv_addr2, serv_addr3);
    p_espudp->dest_port = port;

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
    // printf("Total sent size %u\n", (unsigned int)total_size);
    std::unique_ptr<AsyncUDPMessage> wt_pkt(new AsyncUDPMessage(total_size));
    wt_pkt->write(p_send_data, total_size);
    // n_write = p_espudp->p_udp_client->writeTo(p_send_data, total_size, p_espudp->dest_ip_addr, p_espudp->dest_port);
    n_write = p_espudp->p_udp_client->sendTo(*wt_pkt, p_espudp->dest_ip_addr, p_espudp->dest_port, TCPIP_ADAPTER_IF_MAX);
    if (n_write != total_size)
    {
        printf("Error transmitting packet. Total size attempted to transmit %d, actual size transmitted %d\n", total_size, n_write);
    }
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
    blob_jbuf_release_latest_entry(p_state->p_blob_jbuf);
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