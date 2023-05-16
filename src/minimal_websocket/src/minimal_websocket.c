#ifdef BLOB_WEBSOCKETS
#include <libwebsockets.h>
#include "src/minimal_websocket/include/minimal_websocket.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define EXAMPLE_RX_BUFFER_BYTES (4096)


typedef struct tx_rx_info_s {
	void *p_tx_data;
	size_t n_max_tx_data;
	void *p_rx_data;
	size_t n_max_rx_data;
	size_t n_tx_current_data;
	size_t n_rx_current_data;
} tx_rx_info;


typedef struct minimal_websocket_s {
	struct lws_context *p_context;
	time_t old;
	struct lws *web_socket;
	tx_rx_info *p_tx_rx_info;
	unsigned int port;
	char address[256];
	size_t max_tx_size;
	size_t max_rx_size;
	int b_overwrite_on_send; /* Overwrite rather than append to send buffer */
	int b_new_data;
} minimal_websocket_s;

static int servicing_callback( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	minimal_websocket *p_min_ws;
	switch( reason )
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			printf("Client established.\n");
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			/* Handle incomming messages here. */
			p_min_ws = (minimal_websocket*)lws_get_protocol(wsi)->user;
			printf("-- Received a packet--\n");
			if (len > p_min_ws->p_tx_rx_info->n_max_rx_data)
			{
				printf("Error, rx buffer size not large enough.\n");
				break;
			}
			memcpy(p_min_ws->p_tx_rx_info->p_rx_data, in, len);
			p_min_ws->p_tx_rx_info->n_rx_current_data = len;
			p_min_ws->b_new_data = 1;
			lws_set_timer_usecs(p_min_ws->web_socket, -1);
			break;
		case LWS_CALLBACK_CLIENT_WRITEABLE:
			p_min_ws = (minimal_websocket*)lws_get_protocol(wsi)->user;
			tx_rx_info *p_tx_rx_info = p_min_ws->p_tx_rx_info;
			printf("Client writeable.\n");
			lws_write( wsi, &(((unsigned char*)p_tx_rx_info->p_tx_data)[LWS_SEND_BUFFER_PRE_PADDING]), p_tx_rx_info->n_tx_current_data, LWS_WRITE_BINARY );

			memset(&(((unsigned char*)p_tx_rx_info->p_tx_data)[LWS_SEND_BUFFER_PRE_PADDING]), 0, p_tx_rx_info->n_tx_current_data);
			p_tx_rx_info->n_tx_current_data = 0;
			break;
		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			p_min_ws = (minimal_websocket*)lws_get_protocol(wsi)->user;
			printf("Error connecting to server.\n");
			char errbuf[256] = {0};
			if (NULL != in)
			{
				memcpy(errbuf, in, len);
				printf("%s", errbuf);
			}
			p_min_ws->web_socket = NULL;
			break;
		case LWS_CALLBACK_TIMER:
			p_min_ws = (minimal_websocket*)lws_get_protocol(wsi)->user;
			lws_set_timer_usecs(p_min_ws->web_socket, -1);
			lws_cancel_service(p_min_ws->p_context);
		default:
			//printf("reason: %d\n", reason);
			break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_EXAMPLE = 0,
	PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
	{
		"protocol-1",
		servicing_callback,
		0, /* per session data size */
		EXAMPLE_RX_BUFFER_BYTES,
		0, /* id  */
		NULL, /* user */
		EXAMPLE_RX_BUFFER_BYTES /* tx packet size */
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

int
minimal_websocket_init(minimal_websocket **pp_self, minimal_websocket_cfg *p_cfg)
{
	struct lws_context_creation_info info;
	memset( &info, 0, sizeof(info) );

	*pp_self = (minimal_websocket*)malloc(sizeof(minimal_websocket));

	(*pp_self)->p_tx_rx_info = (tx_rx_info*)malloc(sizeof(tx_rx_info));
	(*pp_self)->p_tx_rx_info->p_tx_data = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + p_cfg->max_tx_size + LWS_SEND_BUFFER_POST_PADDING);
	(*pp_self)->p_tx_rx_info->p_rx_data = (unsigned char*)malloc(p_cfg->max_rx_size);
	
	/* Always transmit the maximum number of bytes; lots of redundancy for now! */
	(*pp_self)->p_tx_rx_info->n_tx_current_data = 0;
	(*pp_self)->p_tx_rx_info->n_rx_current_data = 0;
	(*pp_self)->p_tx_rx_info->n_max_tx_data = p_cfg->max_tx_size;
	(*pp_self)->p_tx_rx_info->n_max_rx_data = p_cfg->max_rx_size;

	protocols[0].user = (*pp_self);

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	strcpy((*pp_self)->address, p_cfg->addr);
	(*pp_self)->port = p_cfg->port;
	(*pp_self)->b_overwrite_on_send = p_cfg->b_overwrite_on_send;

	(*pp_self)->p_context = lws_create_context( &info );
	(*pp_self)->old = 0;

	{
		struct lws_client_connect_info ccinfo = {0};
		ccinfo.context = (*pp_self)->p_context;
		ccinfo.address = (*pp_self)->address;
		ccinfo.port = (*pp_self)->port;
		ccinfo.path = "/";
		ccinfo.host = (*pp_self)->address;
		ccinfo.origin = (*pp_self)->address;
		ccinfo.protocol = protocols[PROTOCOL_EXAMPLE].name;
		ccinfo.pwsi = &(*pp_self)->web_socket;
		lws_client_connect_via_info(&ccinfo);
		if (NULL == (*pp_self)->web_socket)
		{
			printf("Error creating the client!\n");
		}
		else
		{
			printf("Probably connected OK\n");
		}
	}

	return 0;
}

void
minimal_websocket_set_send_data(minimal_websocket *p_self,
								unsigned char *p_data,
								size_t n_bytes
								)
{
	if (p_self->b_overwrite_on_send)
	{
		if (p_self->p_tx_rx_info->n_max_tx_data < n_bytes)
		{
			printf("Maximum tx size of %u allowed, but %u bytes were requested.\n", (unsigned int)p_self->max_tx_size, (unsigned int)n_bytes);
			assert(0);
			return;
		}
		memset(&(((unsigned char*)p_self->p_tx_rx_info->p_tx_data)[LWS_SEND_BUFFER_PRE_PADDING]), 0, p_self->p_tx_rx_info->n_tx_current_data);
		memcpy(&(((unsigned char*)p_self->p_tx_rx_info->p_tx_data)[LWS_SEND_BUFFER_PRE_PADDING]), p_data, n_bytes);
		p_self->p_tx_rx_info->n_tx_current_data = n_bytes;
	}
	else
	{
		if (p_self->p_tx_rx_info->n_max_tx_data < (n_bytes + p_self->p_tx_rx_info->n_tx_current_data))
		{
			printf("Maximum tx size of %u allowed, but %u bytes were requested. %u bytes were already waiting in queue.", (unsigned int)p_self->max_tx_size, (unsigned int)n_bytes, (unsigned int)p_self->p_tx_rx_info->n_tx_current_data);
			assert(0);
			return;
		}
		else
		{
			memcpy(&(((unsigned char*)p_self->p_tx_rx_info->p_tx_data)[LWS_SEND_BUFFER_PRE_PADDING + p_self->p_tx_rx_info->n_tx_current_data]), p_data, n_bytes);
			p_self->p_tx_rx_info->n_tx_current_data = (n_bytes + p_self->p_tx_rx_info->n_tx_current_data);
		}
	}
	
	printf("Data in buffer: %u\n", (unsigned int)p_self->p_tx_rx_info->n_tx_current_data);
	lws_callback_on_writable( p_self->web_socket );
}

void
minimal_websocket_get_recv_data(minimal_websocket *p_self,
								unsigned char **pp_data,
								size_t *n_bytes
								)
{
	if (p_self->b_new_data)
	{
		/* Return point to websocket buffer */
		*pp_data = p_self->p_tx_rx_info->p_rx_data;
		*n_bytes = p_self->p_tx_rx_info->n_rx_current_data;
		p_self->b_new_data = 0;
	}
	else
	{
		/* Return point to websocket buffer */
		*pp_data = NULL;
		*n_bytes = 0;
	}
}


void
minimal_websocket_service(minimal_websocket *p_self)
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	/* Connect if we are not connected to the server. */
	if( !p_self->web_socket && tv.tv_sec != p_self->old )
	{
		struct lws_client_connect_info ccinfo = {0};
		ccinfo.context = p_self->p_context;
		ccinfo.address = p_self->address;
		ccinfo.port = p_self->port;
		ccinfo.path = "/";
		ccinfo.host = p_self->address;
		ccinfo.origin = p_self->address;
		ccinfo.protocol = protocols[PROTOCOL_EXAMPLE].name;
		ccinfo.pwsi = &p_self->web_socket;
		lws_client_connect_via_info(&ccinfo);
		if (NULL == p_self->web_socket)
		{
			printf("Error creating the client!\n");
		}
		else
		{
			printf("Probably connected OK\n");
		}
	}
	lws_set_timer_usecs(p_self->web_socket, 500);
	lws_service( p_self->p_context, /* timeout_ms = */ 5 );
}

void
minimal_websocket_close(minimal_websocket **pp_self)
{
	lws_context_destroy((*pp_self)->p_context);
	free((*pp_self)->p_tx_rx_info);
	free(*pp_self);
}

#endif