#include <stddef.h>

#define BLOB_JBUF_OVERWROTE_PACKET  (3)
#define BLOB_JBUF_FRAGMENT_RECEIVED (3)
#define BLOB_JBUF_DROPPED_FRAGMENT  (2)
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
blob_jbuf_release_latest_entry(blob_jbuf *p_jbuf);

int
blob_jbuf_close(blob_jbuf **pp_jbuf);
