/* Responsible for send data callback, populating a shared receive data array and */
#ifdef BLOB_WEBSOCKETS
#include "../include/blob.h"
#include "include/blob_comm.h"
#include "minimal_websocket/include/minimal_websocket.h"

int
_blob_minws_init(blob_comm_cfg *p_cfg, const char *addr, int port);

int
_blob_minws_terminate(blob_comm_cfg *p_blob_comm_cfg);

int
_blob_minws_rcv_callback(void *p_context, unsigned char **pp_recv_data, size_t *p_recv_total_size);

int
_blob_minws_send_callback(void *p_context, unsigned char *p_send_data, size_t total_size);

typedef struct blob_minws_state_s
{
    minimal_websocket *p_min_ws;
} blob_minws_state;

int
_blob_minws_init(blob_comm_cfg *p_cfg, const char *addr, int port)
{
    minimal_websocket *p_minws;
    minimal_websocket_cfg cfg;
    cfg.addr = (char*)addr;
    cfg.port = port;
    cfg.max_tx_size = 16384 << 4;
    cfg.max_rx_size = 16384 << 4;
    cfg.b_overwrite_on_send = 1;
    minimal_websocket_init((minimal_websocket**)&p_minws, &cfg);
    
    p_cfg->p_send_cb = _blob_minws_send_callback;
    p_cfg->p_send_context = (void*)p_minws;
    p_cfg->p_rcv_cb = _blob_minws_rcv_callback;
    p_cfg->p_rcv_context = (void*)p_minws;

    return 0;
}

int
_blob_minws_terminate(blob_comm_cfg *p_blob_comm_cfg)
{
    minimal_websocket *p_mws = (minimal_websocket*)(p_blob_comm_cfg->p_send_context);
    minimal_websocket_close(&p_mws);
    return 0;
}

int
_blob_minws_send_callback(void *p_context, unsigned char *p_send_data, size_t total_size)
{
    minimal_websocket_set_send_data((minimal_websocket*)p_context, p_send_data, total_size);
    minimal_websocket_service((minimal_websocket*)p_context);
    return 0;
};

int
_blob_minws_rcv_callback(void *p_context, unsigned char **pp_recv_data, size_t *p_recv_total_size)
{
    minimal_websocket_service((minimal_websocket*)p_context);
    minimal_websocket_get_recv_data((minimal_websocket*)p_context, pp_recv_data, p_recv_total_size);
    return 0;
};
#endif