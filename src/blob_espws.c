/* Responsible for send data callback, populating a shared receive data array and */
#ifdef BLOB_ESP32_WEBSOCKETS
#include "../include/blob.h"
#include "blob_comm.h"
#include "esp_websocket_client.h"


esp_websocket_client_config_t g_ws_cfg;

int
_blob_espws_init(blob_comm_cfg *p_cfg, const char *addr, int port);

int
_blob_espws_terminate(blob_comm_cfg *p_blob_comm_cfg);

int
_blob_espws_rcv_callback(void *p_context, unsigned char **pp_recv_data, size_t *p_recv_total_size);

int
_blob_espws_send_callback(void *p_context, unsigned char *p_send_data, size_t total_size);

static void
websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

typedef struct blob_espws_state_s
{
    esp_websocket_client_handle_t p_espws_client;
    unsigned char *p_data;
    size_t n_data;
} blob_espws_state;

int
_blob_espws_init(blob_comm_cfg *p_cfg, const char *addr, int port)
{
    
    blob_espws_state *p_espws;
    esp_err_t err;

    p_espws = (blob_espws_state*)calloc(sizeof(blob_espws_state), 1);
    g_ws_cfg.uri = addr;
    g_ws_cfg.port = port;
    p_espws->p_espws_client = esp_websocket_client_init(&g_ws_cfg);

    p_espws->n_data = 0;
    p_espws->p_data = NULL;
    esp_websocket_register_events(p_espws->p_espws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)(p_espws));

    err = esp_websocket_client_start(p_espws->p_espws_client);
    if (ESP_OK != err)
    {
        printf("Error establishing connection with server.");
        return -1;
    }
    
    p_cfg->p_send_cb = _blob_espws_send_callback;
    p_cfg->p_send_context = (void*)p_espws;
    p_cfg->p_rcv_cb = _blob_espws_rcv_callback;
    p_cfg->p_rcv_context = (void*)p_espws;

    return 0;
}

int
_blob_espws_terminate(blob_comm_cfg *p_blob_comm_cfg)
{
    esp_err_t err;
    blob_espws_state *p_espws = (blob_espws_state*)p_blob_comm_cfg->p_send_context;
    err = esp_websocket_client_stop(p_espws->p_espws_client);
    esp_websocket_client_destroy(p_espws->p_espws_client);
    return 0;
}

int
_blob_espws_send_callback(void *p_context, unsigned char *p_send_data, size_t total_size)
{
    blob_espws_state *p_state = (blob_espws_state*)p_context;
    int n_sent;
    n_sent = esp_websocket_client_send_bin(p_state->p_espws_client, (char*)p_send_data, total_size, 1000);
    if (n_sent != total_size)
    {
        printf("Error: Full amount of data not transmitted.\n");
    }
    if (n_sent == -1)
    {
        printf("Error: Error calling esp_websocket_client_send.\n");
        return -1;
    }
    return 0;
};

int
_blob_espws_rcv_callback(void *p_context, unsigned char **pp_recv_data, size_t *p_recv_total_size)
{
    blob_espws_state *p_state = (blob_espws_state*)p_context;
    *pp_recv_data = p_state->p_data;
    *p_recv_total_size = p_state->n_data;
    return 0;
};


static void
websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    blob_espws_state *p_state = (blob_espws_state*)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t*)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        printf("WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        printf("WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        printf("WEBSOCKET_EVENT_DATA\n");
        printf("Received opcode=%d\n", data->op_code);
        printf("Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        /* Only if the event is a data receive... Probably don't need to do a copy here with extra considerations. */
        if (data->op_code == 2)
        {
            if (NULL != p_state->p_data)
            {
                free(p_state->p_data);
            }
            p_state->p_data = (unsigned char*)malloc(data->data_len);
            memcpy(p_state->p_data, data->data_ptr, data->data_len);
            p_state->n_data = data->data_len;
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        printf("WEBSOCKET_EVENT_ERROR");
        break;
    }
}
#endif