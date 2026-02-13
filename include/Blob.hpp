#ifndef BLOB_H
#define BLOB_H

#define BLOB_OK          (0)
#define BLOB_ERR         (-1)

extern "C"
{
    #include "blob_comm.h"
    typedef struct blob_s blob;

    int
    blob_init(blob **pp_blob, blob_comm_cfg *p_blob_comm_cfg);

    int
    blob_close(blob **pp_blob);

    int
    blob_start(blob *p_blob, const char *node_name);

    int
    blob_float_a(blob *p_blob, const char *var_name, float *p_var_val, int n);

    int
    blob_int_a(blob *p_blob, const char *var_name, int *p_var_val, int n);

    int
    blob_unsigned_int_a(blob *p_blob, const char *var_name, unsigned int *p_var_val, int n);

    int
    blob_flush(blob *p_blob);

    int
    blob_retrieve_start(blob *p_blob, const char* node_name);

    int
    blob_retrieve_float_a(blob *p_blob, const char *var_name, const float **pp_var_val, int *p_n, int rep);

    int
    blob_retrieve_int_a(blob *p_blob, const char *var_name, const int **pp_var_val, int *p_n, int rep);

    int
    blob_retrieve_unsigned_int_a(blob *p_blob, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep);

    int
    blob_retrieve_flush(blob *p_blob);
}

#endif
