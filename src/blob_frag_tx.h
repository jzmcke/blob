#include <stddef.h>

typedef struct blob_frag_tx_s blob_frag_tx;

#ifdef BLOB_WINDOWS_DLL
__declspec(dllexport)
#endif
int
blob_frag_tx_init(blob_frag_tx **pp_blob_frag_tx, size_t frag_size);

#ifdef BLOB_WINDOWS_DLL
__declspec(dllexport)
#endif
int
blob_frag_tx_begin_packet(blob_frag_tx *p_blob_frag_tx, unsigned char *p_data, size_t n);

#ifdef BLOB_WINDOWS_DLL
__declspec(dllexport)
#endif
int
blob_frag_tx_next_packet(blob_frag_tx *p_blob_frag_tx, unsigned char **pp_data, size_t *p_n);
