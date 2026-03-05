#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob_ws_win.h"

#pragma comment(lib, "ws2_32.lib")

// Fallback for macros if header is being weird
#ifndef FD_ZERO
#define FD_ZERO(set) (((fd_set FAR *)(set))->fd_count=0)
#endif
#ifndef FD_SET
#define FD_SET(fd, set) do { \
    u_int __i; \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == (fd)) { \
            break; \
        } \
    } \
    if (__i == ((fd_set FAR *)(set))->fd_count) { \
        if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE) { \
            ((fd_set FAR *)(set))->fd_array[__i] = (fd); \
            ((fd_set FAR *)(set))->fd_count++; \
        } \
    } \
} while(0)
#endif

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

    // Persistent receive buffer for raw stream
    unsigned char raw_recv_buffer[4 * 1024 * 1024];
    size_t raw_recv_len;

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
    char temp_buffer[1024];
    int received = recv(ctx->sock, temp_buffer, sizeof(temp_buffer), 0);
    
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
    
    // Append to raw buffer
    if (ctx->raw_recv_len + received <= sizeof(ctx->raw_recv_buffer)) {
        memcpy(ctx->raw_recv_buffer + ctx->raw_recv_len, temp_buffer, received);
        ctx->raw_recv_len += received;
    } else {
        return -1; // Buffer too small for handshake? Unexpected.
    }

    // Ensure null-terminated for strstr (use a temporary check on raw_recv_buffer)
    // We search for the end of headers: \r\n\r\n
    unsigned char *p_end = NULL;
    for (size_t i = 0; i + 3 < ctx->raw_recv_len; i++) {
        if (ctx->raw_recv_buffer[i] == '\r' && ctx->raw_recv_buffer[i+1] == '\n' &&
            ctx->raw_recv_buffer[i+2] == '\r' && ctx->raw_recv_buffer[i+3] == '\n') {
            p_end = ctx->raw_recv_buffer + i + 4;
            break;
        }
    }

    if (p_end) {
        // Found end of headers. Check if it's a 101.
        // Temporarily null terminate at p_end to use strstr safely
        unsigned char saved = *p_end;
        *p_end = '\0';
        int is_101 = strstr((char*)ctx->raw_recv_buffer, "101") && strstr((char*)ctx->raw_recv_buffer, "Switching Protocols");
        *p_end = saved;

        if (is_101) {
            ctx->state = WS_STATE_CONNECTED;
            printf("WebSocket handshake successful\n");
            
            // Remove headers from buffer, keep any following data
            size_t header_len = p_end - ctx->raw_recv_buffer;
            if (ctx->raw_recv_len > header_len) {
                memmove(ctx->raw_recv_buffer, p_end, ctx->raw_recv_len - header_len);
            }
            ctx->raw_recv_len -= header_len;
            return 1; // Success
        } else {
            printf("WebSocket invalid handshake response\n");
            return -1;
        }
    }
    
    return 0; // Still waiting for more header data
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
    
    // If the first byte is not a valid start of a binary frame (0x80 | 0x02 = 0x82), 
    // we might have lost sync. In a real TCP stream this shouldn't happen, 
    // but if we are at sync, it should be 0x81 (text) or 0x82 (binary).
    if ((in[0] & 0x70) != 0) {
        // Unexpected reserved bits. This is likely not a frame start.
        return -1; 
    }

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
    
    // Safety check for output buffer
    if (payload_len > WS_BUFFER_SIZE) {
        printf("WebSocket frame too large for buffer: %u bytes\n", (unsigned int)payload_len);
        return -2; // Fatal error
    }

    // Extract payload (handle masking if present, though server shouldn't mask)
    const unsigned char *payload = in + header_len + (masked ? 4 : 0);
    if (payload_len > 0 && out) {
        if (masked) {
            const unsigned char *mask = in + header_len;
            for (size_t i = 0; i < payload_len; i++) {
                out[i] = payload[i] ^ mask[i % 4];
            }
        } else {
            memcpy(out, payload, payload_len);
        }
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
    
    // Attempt to decode a frame from the raw buffer
    while (ctx->raw_recv_len > 0) {
        size_t payload_len = 0;
        int frame_size = decode_ws_frame(ctx->raw_recv_buffer, ctx->raw_recv_len, ctx->recv_buffer, &payload_len);
        
        if (frame_size > 0) {
            // Found a complete frame
            *pp_data = ctx->recv_buffer;
            *p_size = payload_len;
            
            // Remove the frame from the raw buffer
            if (ctx->raw_recv_len > (size_t)frame_size) {
                memmove(ctx->raw_recv_buffer, ctx->raw_recv_buffer + frame_size, ctx->raw_recv_len - frame_size);
            }
            ctx->raw_recv_len -= frame_size;
            
            return 0;
        } else if (frame_size < 0) {
            // Error or out of sync. Search for next frame start (0x81 or 0x82)
            printf("WebSocket out of sync! Searching for next frame...\n");
            size_t shift = 1;
            while (shift < ctx->raw_recv_len) {
                if (ctx->raw_recv_buffer[shift] == 0x81 || ctx->raw_recv_buffer[shift] == 0x82) {
                    break;
                }
                shift++;
            }
            memmove(ctx->raw_recv_buffer, ctx->raw_recv_buffer + shift, ctx->raw_recv_len - shift);
            ctx->raw_recv_len -= shift;
            // Loop and try again with the new start
        } else {
            // Incomplete frame, wait for more data
            return 0;
        }
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
            ctx->raw_recv_len = 0; // Clear buffer on new connection
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
            // Try to receive all available data
            unsigned char temp_buffer[WS_BUFFER_SIZE];
            int received;
            while ((received = recv(ctx->sock, (char*)temp_buffer, sizeof(temp_buffer), 0)) > 0) {
                // Append to raw buffer if there's space
                if (ctx->raw_recv_len + received <= sizeof(ctx->raw_recv_buffer)) {
                    memcpy(ctx->raw_recv_buffer + ctx->raw_recv_len, temp_buffer, received);
                    ctx->raw_recv_len += received;
                } else {
                    printf("WebSocket raw receive buffer overflow!\n");
                    ctx->raw_recv_len = 0; // Reset on overflow to try to recover
                    break;
                }
            }
            
            if (received == 0) {
                // Connection closed
                ctx->state = WS_STATE_DISCONNECTED;
                closesocket(ctx->sock);
                ctx->sock = INVALID_SOCKET;
            } else if (received == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    ctx->state = WS_STATE_ERROR;
                }
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
