typedef struct blob_frag_tx_s
{
    unsigned char *p_data;
    size_t data_size;
    size_t n_remaining;
    size_t n_total_written;
    size_t frag_size;
    int frag_idx;
    int seq_num;
    int n_frags;
    unsigned char *p_out_buffer;
} blob_frag_tx;

#ifdef BLOB_WINDOWS_DLL
__declspec(dllexport)
#endif
int
blob_frag_tx_init(blob_frag_tx *p_blob_frag_tx, unsigned char *p_data, size_t n);

#ifdef BLOB_WINDOWS_DLL
__declspec(dllexport)
#endif
int
blob_frag_tx_begin_packet(blob_frag_tx *p_blob_frag_tx, unsigned char **pp_data, size_t *p_n);

#ifdef BLOB_WINDOWS_DLL
__declspec(dllexport)
#endif
int
blob_frag_tx_next_packet(blob_frag_tx *p_blob_frag_tx, unsigned char **pp_data, size_t *p_n);