#ifndef BLOB_CORE_H
#define BLOB_CORE_H
#include <stdio.h>

#define BLOB_PROTOCOL_VERSION_FIRST (1)
#define BLOB_MAX_VAR_NAME_LEN       (128)

#define BLOB_MAX_VARS_PER_BLOB      (32)
#define BLOB_VAR_TYPE_INT           (0)
#define BLOB_VAR_TYPE_FLOAT         (1)
#define BLOB_VAR_TYPE_UNSIGNED_INT  (2)

#define BLOB_ERR                    (-1)
#define BLOB_OK                     (0)

typedef struct blob_core_s blob_core;

typedef struct blob_core_cfg_s
{
    unsigned int protocol;
} blob_core_cfg;

int
blob_core_close(blob_core **pp_blob);

int
blob_core_init(blob_core **pp_blob, blob_core_cfg *p_cfg);

int
blob_core_float_a(blob_core *p_blob, const char *p_var_name, float *p_var_val, int n);

int
blob_core_int_a(blob_core *p_blob, const char *p_var_name, int *p_var_val, int n);

int
blob_core_unsigned_int_a(blob_core *p_blob, const char *p_var_name, unsigned int *p_var_val, int n);

void
blob_core_get_data(blob_core *p_blob, unsigned char **pp_data, size_t *blob_size);

void
blob_core_get_info(blob_core *p_blob, int **pp_blob_len, int **pp_var_types, char **pp_var_names, int *n_vars, int *n_repititions);

int
blob_core_retrieve_float_a(blob_core *p_blob, const char *var_name, const float **pp_var_val, int *p_n, int rep);

int
blob_core_retrieve_int_a(blob_core *p_blob, const char *var_name, const int **pp_var_val, int *p_n, int rep);

int
blob_core_retrieve_unsigned_int_a(blob_core *p_blob, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep);

size_t
blob_core_set_from_data(blob_core *p_blob, unsigned char *p_data, size_t *p_total_size);

void
blob_core_update_root_data(blob_core *p_blob, unsigned char *p_data);

size_t
blob_core_get_serialized_data_size(blob_core *p_blob);

int
blob_core_header_get_size(blob_core *p_blob, size_t *p_size);

#endif
