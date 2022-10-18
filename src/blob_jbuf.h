#include <stddef.h>

#define BLOB_JBUF_DROPPED_PACKET (1)
#define BLOB_JBUF_ERR            (-1)
#define BLOB_JBUF_OK             (0)

typedef struct blob_jbuf_s blob_jbuf;

typedef struct blob_jbuf_cfg_s
{
    int jbuf_len;
} blob_jbuf_cfg;

int
blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg);

int
blob_jbuf_push(blob_jbuf *p_jbuf, void *p_new_data, size_t n);

int
blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_data, size_t *p_n);

int
blob_jbuf_close(blob_jbuf **pp_jbuf);
