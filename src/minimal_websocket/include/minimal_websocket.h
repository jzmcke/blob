#ifdef BLOB_WEBSOCKETS
#include <stdlib.h>

typedef struct minimal_websocket_s minimal_websocket;

typedef struct minimal_websocket_cfg
{
    char *addr;
    int port;
    size_t max_tx_size;
    size_t max_rx_size;
    int b_overwrite_on_send;
} minimal_websocket_cfg;

void
minimal_websocket_get_recv_data(minimal_websocket *p_self, unsigned char **pp_data, size_t *n_bytes);

int
minimal_websocket_init(minimal_websocket **pp_self, minimal_websocket_cfg *p_cfg);

void
minimal_websocket_set_send_data(minimal_websocket *p_self, unsigned char *p_data, size_t n_bytes);

void
minimal_websocket_service(minimal_websocket *p_self);

void
minimal_websocket_close(minimal_websocket **pp_self);

#endif