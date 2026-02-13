#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "blob_ws_win.h"

#pragma comment(lib, "ws2_32.lib")

#define WS_BUFFER_SIZE 65536
#define WS_HANDSHAKE_TIMEOUT_MS 5000

typedef enum {
    WS_STATE_DISCONNECTED,
    WS_STATE_CONNECTING,
    WS_STATE_CONNECTED,
    WS_STATE_ERROR
} ws_state_t;

typedef struct {
    SOCKET sock;
    ws_state_t state;
    char host[256];
    int port;
    
    // Send/receive buffers
    unsigned char send_buffer[WS_BUFFER_SIZE];
    unsigned char recv_buffer[WS_BUFFER_SIZE];
    size_t recv_len;
    int b_has_data;
    
    // Handshake tracking
    int handshake_sent;
    DWORD connect_start_time;
} ws_context_t;

// Generate a simple WebSocket key (base64 encoded random bytes)
static void generate_ws_key(char *key_out, size_t key_size) {
    const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (size_t i = 0; i < key_size - 1 && i < 24; i++) {
        key_out[i] = base64_chars[rand() % 64];
    }
    key_out[23] = '\0';
}

// Send WebSocket handshake
static int send_handshake(ws_context_t *ctx) {
    char handshake[512];
    char ws_key[25];
    
    generate_ws_key(ws_key, sizeof(ws_key));
    
    snprintf(handshake, sizeof(handshake),
        "GET / HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        ctx->host, ctx->port, ws_key);
    
    int sent = send(ctx->sock, handshake, (int)strlen(handshake), 0);
    if (sent == SOCKET_ERROR) {
        return -1;
    }
    
    ctx->handshake_sent = 1;
    ctx->connect_start_time = GetTickCount();
    return 0;
}

// Check if handshake response is complete
static int check_handshake_response(ws_context_t *ctx) {
    char buffer[1024];
    int received = recv(ctx->sock, buffer, sizeof(buffer) - 1, 0);
    
    if (received == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            // Check timeout
            if (GetTickCount() - ctx->connect_start_time > WS_HANDSHAKE_TIMEOUT_MS) {
                printf("WebSocket handshake timeout\n");
                return -1; // Timeout
            }
            return 0; // Still waiting
        }
        printf("WebSocket handshake recv error: %d\n", err);
        return -1; // Error
    }
    
    if (received == 0) {
        printf("WebSocket connection closed during handshake\n");
        return -1; // Connection closed
    }
    
    buffer[received] = '\0';
    
    // Simple check for "101 Switching Protocols"
    if (strstr(buffer, "101") && strstr(buffer, "Switching Protocols")) {
        ctx->state = WS_STATE_CONNECTED;
        printf("WebSocket handshake successful\n");
        return 1; // Success
    }
    
    printf("WebSocket invalid handshake response\n");
    return -1; // Invalid response
}

// Encode WebSocket frame (binary, with masking)
static int encode_ws_frame(unsigned char *out, const unsigned char *data, size_t data_len) {
    size_t frame_len = 0;
    
    // Opcode: 0x82 (binary frame, FIN=1)
    out[frame_len++] = 0x82;
    
    // Payload length with mask bit
    if (data_len < 126) {
        out[frame_len++] = 0x80 | (unsigned char)data_len;
    } else if (data_len < 65536) {
        out[frame_len++] = 0x80 | 126;
        out[frame_len++] = (data_len >> 8) & 0xFF;
        out[frame_len++] = data_len & 0xFF;
    } else {
        // For larger payloads, use 8-byte length
        out[frame_len++] = 0x80 | 127;
        for (int i = 7; i >= 0; i--) {
            out[frame_len++] = (data_len >> (i * 8)) & 0xFF;
        }
    }
    
    // Masking key (simple random)
    unsigned char mask[4];
    for (int i = 0; i < 4; i++) {
        mask[i] = rand() & 0xFF;
        out[frame_len++] = mask[i];
    }
    
    // Masked payload
    for (size_t i = 0; i < data_len; i++) {
        out[frame_len++] = data[i] ^ mask[i % 4];
    }
    
    return (int)frame_len;
}

// Decode WebSocket frame (simple binary frame decoder)
static int decode_ws_frame(const unsigned char *in, size_t in_len, unsigned char *out, size_t *out_len) {
    if (in_len < 2) return 0; // Need at least 2 bytes
    
    unsigned char opcode = in[0] & 0x0F;
    int masked = (in[1] & 0x80) != 0;
    size_t payload_len = in[1] & 0x7F;
    size_t header_len = 2;
    
    // Extended payload length
    if (payload_len == 126) {
        if (in_len < 4) return 0;
        payload_len = (in[2] << 8) | in[3];
        header_len = 4;
    } else if (payload_len == 127) {
        if (in_len < 10) return 0;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | in[2 + i];
        }
        header_len = 10;
    }
    
    // Check if we have the full frame
    size_t total_len = header_len + (masked ? 4 : 0) + payload_len;
    if (in_len < total_len) return 0; // Incomplete frame
    
    // Extract payload (handle masking if present, though server shouldn't mask)
    const unsigned char *payload = in + header_len + (masked ? 4 : 0);
    if (payload_len > 0 && out) {
        memcpy(out, payload, payload_len);
        *out_len = payload_len;
    }
    
    return (int)total_len; // Return total frame size
}

// Send callback
static int ws_send_callback(void *context, unsigned char *data, size_t size) {
    ws_context_t *ctx = (ws_context_t*)context;
    
    if (ctx->state != WS_STATE_CONNECTED) {
        return -1;
    }
    
    // Encode as WebSocket frame
    int frame_len = encode_ws_frame(ctx->send_buffer, data, size);
    
    // Send frame
    int sent = send(ctx->sock, (const char*)ctx->send_buffer, frame_len, 0);
    if (sent == SOCKET_ERROR) {
        return -1;
    }
    
    return 0;
}

// Receive callback
static int ws_recv_callback(void *context, unsigned char **pp_data, size_t *p_size) {
    ws_context_t *ctx = (ws_context_t*)context;
    
    *pp_data = NULL;
    *p_size = 0;
    
    if (ctx->state != WS_STATE_CONNECTED) {
        return -1;
    }
    
    // Return previously received data if available
    if (ctx->b_has_data) {
        *pp_data = ctx->recv_buffer;
        *p_size = ctx->recv_len;
        ctx->b_has_data = 0;
        return 0;
    }
    
    return 0;
}

int blob_ws_win_init(blob_comm_cfg *p_cfg, const char *addr, int port) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return -1;
    }
    
    ws_context_t *ctx = (ws_context_t*)calloc(1, sizeof(ws_context_t));
    if (!ctx) {
        WSACleanup();
        return -1;
    }
    
    strncpy_s(ctx->host, sizeof(ctx->host), addr, _TRUNCATE);
    ctx->port = port;
    ctx->state = WS_STATE_DISCONNECTED;
    ctx->sock = INVALID_SOCKET;
    
    // Set up callbacks
    p_cfg->p_send_cb = ws_send_callback;
    p_cfg->p_send_context = ctx;
    p_cfg->p_rcv_cb = ws_recv_callback;
    p_cfg->p_rcv_context = ctx;
    
    return 0;
}

int blob_ws_win_service(blob_comm_cfg *p_cfg) {
    ws_context_t *ctx = (ws_context_t*)p_cfg->p_send_context;
    if (!ctx) return -1;
    
    switch (ctx->state) {
        case WS_STATE_DISCONNECTED: {
            printf("Attempting WebSocket connection to %s:%d...\n", ctx->host, ctx->port);
            
            // Create socket
            ctx->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (ctx->sock == INVALID_SOCKET) {
                printf("Failed to create socket\n");
                return -1;
            }
            
            // Set non-blocking
            u_long mode = 1;
            ioctlsocket(ctx->sock, FIONBIO, &mode);
            
            // Connect
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(ctx->port);
            inet_pton(AF_INET, ctx->host, &server_addr.sin_addr);
            
            connect(ctx->sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
            ctx->state = WS_STATE_CONNECTING;
            ctx->handshake_sent = 0;
            break;
        }
        
        case WS_STATE_CONNECTING: {
            // Check if connection is established
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(ctx->sock, &write_fds);
            
            struct timeval tv = {0, 0};
            int result = select(0, NULL, &write_fds, NULL, &tv);
            
            if (result > 0 && FD_ISSET(ctx->sock, &write_fds)) {
                // Connection established, send handshake
                if (!ctx->handshake_sent) {
                    if (send_handshake(ctx) < 0) {
                        ctx->state = WS_STATE_ERROR;
                        return -1;
                    }
                } else {
                    // Check for handshake response
                    int hs_result = check_handshake_response(ctx);
                    if (hs_result < 0) {
                        ctx->state = WS_STATE_ERROR;
                        return -1;
                    } else if (hs_result > 0) {
                        printf("WebSocket connected to %s:%d\n", ctx->host, ctx->port);
                    }
                }
            }
            break;
        }
        
        case WS_STATE_CONNECTED: {
            // Try to receive data
            unsigned char temp_buffer[WS_BUFFER_SIZE];
            int received = recv(ctx->sock, (char*)temp_buffer, sizeof(temp_buffer), 0);
            
            if (received > 0) {
                // Decode WebSocket frame
                size_t payload_len = 0;
                int frame_size = decode_ws_frame(temp_buffer, received, ctx->recv_buffer, &payload_len);
                
                if (frame_size > 0 && payload_len > 0) {
                    ctx->recv_len = payload_len;
                    ctx->b_has_data = 1;
                }
            } else if (received == 0) {
                // Connection closed
                ctx->state = WS_STATE_DISCONNECTED;
                closesocket(ctx->sock);
                ctx->sock = INVALID_SOCKET;
            }
            break;
        }
        
        case WS_STATE_ERROR:
            // Attempt reconnection
            if (ctx->sock != INVALID_SOCKET) {
                closesocket(ctx->sock);
                ctx->sock = INVALID_SOCKET;
            }
            ctx->state = WS_STATE_DISCONNECTED;
            break;
    }
    
    return 0;
}

int blob_ws_win_terminate(blob_comm_cfg *p_cfg) {
    if (!p_cfg || !p_cfg->p_send_context) return 0;
    
    ws_context_t *ctx = (ws_context_t*)p_cfg->p_send_context;
    
    if (ctx->sock != INVALID_SOCKET) {
        closesocket(ctx->sock);
    }
    
    free(ctx);
    p_cfg->p_send_context = NULL;
    p_cfg->p_rcv_context = NULL;
    
    WSACleanup();
    return 0;
}

#endif // _WIN32
