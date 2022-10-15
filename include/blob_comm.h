#ifndef BLOB_COMM_H
#define BLOB_COMM_H

#include <stddef.h>

typedef struct blob_comm_cfg_s
{
    int       (*p_send_cb)(void*, unsigned char*, size_t);
    void       *p_send_context;
    int       (*p_rcv_cb)(void*, unsigned char**, size_t*);
    void       *p_rcv_context;
} blob_comm_cfg;

#endif
