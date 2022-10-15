#ifdef BLOB_FILE
#include "blob_core.h"
#include "blob.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int _blob_file_start(char *blob_name);
int _blob_file_float_a(char *var_name, float *p_var_val, int n);
int _blob_file_int_a(char *var_name, int *p_var_val, int n);
int _blob_file_unsigned_int_a(char *var_name, unsigned int *p_var_val, int n);
int _blob_file_flush();

#define MAX_BLOB_FILES   (128)
#define MAX_FILENAME_LEN (128)

struct blob_file_s
{
    blob *p_blob;
    char  p_name[MAX_FILENAME_LEN];
    int b_already_opened;
};

typedef struct blob_file_s blob_file;

struct blob_files_state_s
{
    int        n_files;
    blob_file  p_blob_files[MAX_BLOB_FILES];
    char *p_current_name;
    int current_file_idx;
};

typedef struct blob_files_state_s blob_files_state;

blob_files_state blob_fs = {0};

int
_blob_file_start(char *fname)
{
    blob_cfg cfg;

    cfg.protocol = BLOB_PROTOCOL_VERSION_FIRST;
    int b_found_existing = 0;

    if (NULL != blob_fs.p_current_name)
    {
        /* File must be ended before the next begins! I.e. doesn't support nested statements. */
        return BLOB_ERR;
    }

    for (int i=0; i<blob_fs.n_files; i++)
    {
        if (0 == strcmp(fname, blob_fs.p_blob_files[i].p_name))
        {
            b_found_existing = 1;
            blob_fs.current_file_idx = i;
        }
    }

    if (!b_found_existing)
    {
        if ((blob_fs.n_files + 1) > MAX_BLOB_FILES)
        {
            return BLOB_ERR;
        }

        strcpy(blob_fs.p_blob_files[blob_fs.n_files].p_name, fname);
        blob_init(&blob_fs.p_blob_files[blob_fs.n_files].p_blob, &cfg);
        blob_fs.current_file_idx = blob_fs.n_files;

        blob_fs.n_files++;
    }

    blob_fs.p_current_name = blob_fs.p_blob_files[blob_fs.current_file_idx].p_name;
    return BLOB_OK;
}

int
_blob_file_float_a(char *var_name, float *p_var_val, int n)
{
    int ret;
    if (NULL == blob_fs.p_current_name)
    {
        return BLOB_ERR;
    }

    ret = blob_float_a(blob_fs.p_blob_files[blob_fs.current_file_idx].p_blob, var_name, p_var_val, n);

    return ret;
}

int
_blob_file_int_a(char *var_name, int *p_var_val, int n)
{
    int ret;
    if (NULL == blob_fs.p_current_name)
    {
        return BLOB_ERR;
    }

    ret = blob_int_a(blob_fs.p_blob_files[blob_fs.current_file_idx].p_blob, var_name, p_var_val, n);

    return ret;
}

int
_blob_file_unsigned_int_a(char *var_name, unsigned int *p_var_val, int n)
{
    int ret;
    if (NULL == blob_fs.p_current_name)
    {
        return BLOB_ERR;
    }

    ret = blob_unsigned_int_a(blob_fs.p_blob_files[blob_fs.current_file_idx].p_blob, var_name, p_var_val, n);

    return ret;
}

int
__blob_file_write_hdr(FILE  *p_file,
                      int   *p_blob_len,
                      int   *p_var_types,
                      char  *p_var_names,
                      int    n_vars,
                      int    n_repetitions)
{
    u_int32_t protocol = BLOB_PROTOCOL_VERSION_FIRST;
    u_int32_t vars = n_vars;
    fwrite(&protocol, sizeof(u_int32_t), 1, p_file);
    fwrite(&vars, sizeof(u_int32_t), 1, p_file);
    for (int i=0; i<n_vars; i++)
    {
        fwrite(&p_var_types[i], sizeof(int32_t), 1, p_file);
        fwrite(&p_blob_len[i], sizeof(int32_t), 1, p_file);
        fwrite(&p_var_names[i*BLOB_MAX_VAR_NAME_LEN], sizeof(char), BLOB_MAX_VAR_NAME_LEN, p_file);
    }
}

int
_blob_file_flush()
{
    FILE *p_file;
    unsigned char *p_blob_data;
    size_t blob_size;
    /* Write to file */
    if (!blob_fs.p_blob_files[blob_fs.current_file_idx].b_already_opened)
    {
        int *p_var_len;
        int *p_var_types;
        char *p_var_names;
        int n_vars;
        int n_repetitions;

        /* Overwrite existing files without warning */
        p_file = fopen(blob_fs.p_current_name, "w");

        /* Write header */
        blob_get_info(blob_fs.p_blob_files[blob_fs.current_file_idx].p_blob
                      ,&p_var_len
                      ,&p_var_types
                      ,&p_var_names
                      ,&n_vars
                      ,&n_repetitions);
        __blob_file_write_hdr(p_file, p_var_len, p_var_types, p_var_names, n_vars, n_repetitions);

        blob_fs.p_blob_files[blob_fs.current_file_idx].b_already_opened = 1;
    }
    else
    {
        p_file = fopen(blob_fs.p_current_name, "a");
    }
    blob_get_data(blob_fs.p_blob_files[blob_fs.current_file_idx].p_blob, &p_blob_data, &blob_size);
    fwrite((void*)p_blob_data, sizeof(unsigned char), blob_size, p_file);
    fclose(p_file);
    blob_fs.p_current_name = NULL;
}
#endif