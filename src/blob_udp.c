#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#endif

#include "blob_udp.h"
#include "blob_frag_tx.h"
#include "blob_jbuf_frag.h"

#define ENET_MTU 1400

struct blob_udp_s {
    SOCKET sock;
    struct sockaddr_in dest_addr;
    struct sockaddr_in local_addr;
    blob_frag_tx *p_tx_frag;
    blob_jbuf *p_rx_jbuf;
    unsigned char *p_processed_data; // Pointer to data currently held by app (pulled from jbuf)
    size_t processed_data_size;
    volatile int b_running;
#ifdef _WIN32
    HANDLE hThread;
#else
    pthread_t thread;
#endif
};

// Internal callbacks
static int _blob_udp_send_cb(void *context, unsigned char *data, size_t size);
static int _blob_udp_rcv_cb(void *context, unsigned char **data, size_t *size);
static void _blob_udp_free_cb(unsigned char *p_data, void *p_context) {
    free(p_data);
}

#ifdef _WIN32
static DWORD WINAPI _blob_udp_thread_func(LPVOID lpParam) {
    blob_udp *p_ctx = (blob_udp*)lpParam;
    while (p_ctx->b_running) {
        blob_udp_poll(p_ctx);
        Sleep(1);
    }
    return 0;
}
#else
static void* _blob_udp_thread_func(void *lpParam) {
    blob_udp *p_ctx = (blob_udp*)lpParam;
    while (p_ctx->b_running) {
        blob_udp_poll(p_ctx);
        usleep(1000);
    }
    return NULL;
}
#endif

int blob_udp_init(blob_udp **pp_ctx, blob_comm_cfg *p_cfg, const char *dest_ip, int dest_port, int local_port, int jbuf_len) {
    blob_udp *p_ctx = (blob_udp*)calloc(1, sizeof(blob_udp));
    if (!p_ctx) return -1;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    p_ctx->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (p_ctx->sock == INVALID_SOCKET) {
        free(p_ctx);
        return -1;
    }

    // Bind local
    p_ctx->local_addr.sin_family = AF_INET;
    p_ctx->local_addr.sin_port = htons(local_port);
    p_ctx->local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(p_ctx->sock, (SOCKADDR*)&p_ctx->local_addr, sizeof(p_ctx->local_addr)) == SOCKET_ERROR) {
        closesocket(p_ctx->sock);
        free(p_ctx);
        return -1;
    }

    // Set non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(p_ctx->sock, FIONBIO, &mode);
#else
    int flags = fcntl(p_ctx->sock, F_GETFL, 0);
    fcntl(p_ctx->sock, F_SETFL, flags | O_NONBLOCK);
#endif

    // Dest addr
    if (dest_ip) {
        p_ctx->dest_addr.sin_family = AF_INET;
        p_ctx->dest_addr.sin_port = htons(dest_port);
        p_ctx->dest_addr.sin_addr.s_addr = inet_addr(dest_ip);
    }

    // Components
    blob_frag_tx_init(&p_ctx->p_tx_frag, ENET_MTU);
    
    blob_jbuf_cfg jcfg;
    memset(&jcfg, 0, sizeof(jcfg));
    jcfg.jbuf_len = jbuf_len;
    jcfg.deallocate_callback = _blob_udp_free_cb;
    blob_jbuf_init(&p_ctx->p_rx_jbuf, &jcfg);

    // Config callbacks
    p_cfg->p_send_cb = _blob_udp_send_cb;
    p_cfg->p_send_context = p_ctx;
    p_cfg->p_rcv_cb = _blob_udp_rcv_cb;
    p_cfg->p_rcv_context = p_ctx;

    // Start thread
    p_ctx->b_running = 1;
#ifdef _WIN32
    p_ctx->hThread = CreateThread(NULL, 0, _blob_udp_thread_func, p_ctx, 0, NULL);
#else
    pthread_create(&p_ctx->thread, NULL, _blob_udp_thread_func, p_ctx);
#endif

    *pp_ctx = p_ctx;
    return 0;
}

int blob_udp_close(blob_udp **pp_ctx) {
    if (!pp_ctx || !*pp_ctx) return -1;
    blob_udp *p_ctx = *pp_ctx;
    
    p_ctx->b_running = 0;
#ifdef _WIN32
    if (p_ctx->hThread) {
        WaitForSingleObject(p_ctx->hThread, INFINITE);
        CloseHandle(p_ctx->hThread);
    }
#else
    pthread_join(p_ctx->thread, NULL);
#endif

    if (p_ctx->sock != INVALID_SOCKET) {
        closesocket(p_ctx->sock);
    }
    
    if (p_ctx->p_processed_data) free(p_ctx->p_processed_data);

    // Cleanup components
    blob_frag_tx_close(&p_ctx->p_tx_frag);
    blob_jbuf_close(&p_ctx->p_rx_jbuf);
    
    free(p_ctx);
    *pp_ctx = NULL;
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

int blob_udp_poll(blob_udp *p_ctx) {
    if (!p_ctx) return -1;

    unsigned char buf[2048];
    int len;
    
    // Drain socket
    while (1) {
        len = recvfrom(p_ctx->sock, (char*)buf, sizeof(buf), 0, NULL, NULL);
        if (len > 0) {
            unsigned char *p_data = malloc(len);
            if (p_data) {
                memcpy(p_data, buf, len);
                blob_jbuf_push(p_ctx->p_rx_jbuf, p_data, len);
            }
        } else {
            break; 
        }
    }
    return 0;
}

static int _blob_udp_send_cb(void *context, unsigned char *data, size_t size) {
    blob_udp *p_ctx = (blob_udp*)context;
    
    blob_frag_tx_begin_packet(p_ctx->p_tx_frag, data, size);
    
    unsigned char *p_chunk = NULL;
    size_t chunk_size = 0;
    
    blob_frag_tx_next_packet(p_ctx->p_tx_frag, &p_chunk, &chunk_size);
    while (p_chunk != NULL) {
        sendto(p_ctx->sock, (const char*)p_chunk, (int)chunk_size, 0, (SOCKADDR*)&p_ctx->dest_addr, sizeof(p_ctx->dest_addr));
        blob_frag_tx_next_packet(p_ctx->p_tx_frag, &p_chunk, &chunk_size);
    }
    return 0;
}

static int _blob_udp_rcv_cb(void *context, unsigned char **data, size_t *size) {
    blob_udp *p_ctx = (blob_udp*)context;
    
    void *p_out = NULL;
    size_t out_n = 0;
    
    int res = blob_jbuf_pull(p_ctx->p_rx_jbuf, &p_out, &out_n);
    
    if (res == BLOB_JBUF_OK && p_out) {
        // Compatibility: blob_node_tree_retrieve_start expects 128 bytes of padding
        size_t padding = 128;
        unsigned char *p_padded = (unsigned char*)malloc(out_n + padding);
        if (p_padded) {
            memset(p_padded, 0, padding);
            memcpy(p_padded + padding, p_out, out_n);
            
            // Release old data reference if needed (jbuf will release it on next pull, 
            // but we need to manage our own padded buffer)
            // For now, let's just use a single buffer in the context to avoid leaks
            if (p_ctx->p_processed_data) free(p_ctx->p_processed_data);
            
            p_ctx->p_processed_data = p_padded;
            p_ctx->processed_data_size = out_n + padding;
            
            *data = p_ctx->p_processed_data;
            *size = p_ctx->processed_data_size;
            
            return 0;
        }
    }
    
    *data = NULL;
    *size = 0;
    return 0;
}

int blob_udp_get_n_fragments(blob_udp *p_ctx) {
    if (!p_ctx) return 0;
    return blob_jbuf_get_n_fragments(p_ctx->p_rx_jbuf);
}
