#include "blob_core.h"
#include "blob/include/blob.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct blob_core_s
{
    int             n_vars_in_blob;
    char            aa_var_names[BLOB_MAX_VARS_PER_BLOB][BLOB_MAX_VAR_NAME_LEN];
    int             a_var_types[BLOB_MAX_VARS_PER_BLOB];
    int             a_var_len[BLOB_MAX_VARS_PER_BLOB];
    int             n_repetitions;
    unsigned char  *p_blob_data; /* Blob of memory. Grows with every new allocation that exceeds current size */
    unsigned char  *p_root_blob_data;
    size_t          base_blob_size;
    size_t          total_blob_size;
    unsigned int    a_var_data_offsets[BLOB_MAX_VARS_PER_BLOB]; /* data is allocated on first write. subsequent writes must be the same size. */
    int             var_idx; /* the variable the blob is expecting to be written next */

    size_t          serialized_data_size;
};

int
blob_core_init(blob_core **pp_blob, blob_core_cfg *p_cfg)
{
    *pp_blob = (blob_core*)calloc(sizeof(blob_core), 1);
    (*pp_blob)->n_vars_in_blob = 0;
    (*pp_blob)->var_idx = 0;
    (*pp_blob)->n_repetitions = 0;
    (*pp_blob)->base_blob_size = 0;
    (*pp_blob)->total_blob_size = 0;
    (*pp_blob)->p_blob_data = NULL;
    (*pp_blob)->p_root_blob_data = NULL;
    for (int i=0; i<BLOB_MAX_VARS_PER_BLOB; i++)
    {
        (*pp_blob)->a_var_types[i] = 0;
        (*pp_blob)->a_var_len[i] = 0;
        (*pp_blob)->a_var_data_offsets[i] = 0;
        (*pp_blob)->aa_var_names[i][0] = '\0';
    }
    return BLOB_OK;
}

int
blob_core_close(blob_core **pp_blob)
{
    blob_core *p_blob = *pp_blob;
    if (p_blob != NULL)
    {
        if (NULL != p_blob->p_root_blob_data)
        {
            free(p_blob->p_root_blob_data);
            p_blob->p_root_blob_data = NULL;
        }
        free(*pp_blob);
        *pp_blob = NULL;
    }
    return 0;
}

int
blob_core_float_a(blob_core *p_blob, const char *p_var_name, float *p_var_val, int n)
{
    unsigned char *p_var_data;
    if (  (p_blob->var_idx == p_blob->n_vars_in_blob)
        &&(0 != strcmp(p_var_name, p_blob->aa_var_names[0]))
        )
    {
        unsigned char *p_new_blob;

        /* This occurs only when we are building the first repetition */
        assert(p_blob->n_repetitions == 0);

        /* The variable is new */
        if ((p_blob->n_vars_in_blob + 1) > BLOB_MAX_VARS_PER_BLOB)
        {
            /* Exceeded max size. */
            return BLOB_ERR;
        }

        /* Allocate new, larger blob, copy old blob into it. */
        p_new_blob = (unsigned char*)calloc(p_blob->base_blob_size + sizeof(float)*n, 1);
        memcpy(p_new_blob, p_blob->p_root_blob_data, p_blob->base_blob_size);
        free(p_blob->p_root_blob_data); /* Apparently it's OK to free a NULL pointer.. so the first variable made is all g :) cool */

        p_blob->p_root_blob_data = p_new_blob;
        p_blob->p_blob_data = p_new_blob;
        p_blob->a_var_data_offsets[p_blob->n_vars_in_blob] = p_blob->base_blob_size;
        
        p_blob->base_blob_size += n * sizeof(float);
        p_blob->total_blob_size = p_blob->base_blob_size;
    
        strcpy(p_blob->aa_var_names[p_blob->n_vars_in_blob], p_var_name);
        p_blob->a_var_len[p_blob->n_vars_in_blob] = n;
        p_blob->a_var_types[p_blob->n_vars_in_blob] = BLOB_VAR_TYPE_FLOAT;
        p_blob->n_vars_in_blob++;

    }
    else if (  (p_blob->var_idx == p_blob->n_vars_in_blob)
             &&(0 == strcmp(p_var_name, p_blob->aa_var_names[0]))
             )
    {
        unsigned char *p_new_repetition;
        p_new_repetition = (unsigned char*)calloc(p_blob->total_blob_size + p_blob->base_blob_size, 1);
        memcpy(p_new_repetition, p_blob->p_root_blob_data, p_blob->total_blob_size);
        free(p_blob->p_root_blob_data);
        p_blob->p_root_blob_data = p_new_repetition;
        p_blob->p_blob_data = p_blob->p_root_blob_data + p_blob->total_blob_size;
        p_blob->total_blob_size += p_blob->base_blob_size;
        p_blob->n_repetitions++;
    }
    
    /* If we are over the end, point back to the start */
    p_blob->var_idx = p_blob->var_idx % p_blob->n_vars_in_blob; 

    if (   (0 != strcmp(p_var_name, p_blob->aa_var_names[p_blob->var_idx]))
        || (BLOB_VAR_TYPE_FLOAT != p_blob->a_var_types[p_blob->var_idx])
        || (n != p_blob->a_var_len[p_blob->var_idx])
        )
    {
        /* Unexpected variable write operation */
        return BLOB_ERR;
    }

    p_var_data = &p_blob->p_blob_data[p_blob->a_var_data_offsets[p_blob->var_idx]];
    for (int i=0; i<n; i++)
    {
        *((float*)&p_var_data[i*sizeof(float)]) = (float)p_var_val[i];
    }
    p_blob->var_idx++;
    return BLOB_OK;
}

int
blob_core_int_a(blob_core *p_blob, const char *p_var_name, int *p_var_val, int n)
{
    unsigned char *p_var_data;
    
    if (  (p_blob->var_idx == p_blob->n_vars_in_blob)
        &&(0 != strcmp(p_var_name, p_blob->aa_var_names[0])))
    {
        unsigned char *p_new_blob;

        /* This occurs only when we are building the first repetition */
        assert(p_blob->n_repetitions == 0);

        /* The variable is new */
        if ((p_blob->n_vars_in_blob + 1) > BLOB_MAX_VARS_PER_BLOB)
        {
            /* Exceeded max size. */
            return BLOB_ERR;
        }

        /* Allocate new, larger blob, copy old blob into it. */
        p_new_blob = (unsigned char*)calloc(p_blob->base_blob_size + sizeof(int32_t)*n, 1);
        memcpy(p_new_blob, p_blob->p_blob_data, p_blob->base_blob_size);
        p_blob->a_var_data_offsets[p_blob->n_vars_in_blob] = p_blob->base_blob_size;
        free(p_blob->p_blob_data);
        p_blob->p_blob_data = p_new_blob;
        p_blob->p_root_blob_data = p_new_blob;
        p_blob->base_blob_size += n * sizeof(int32_t);
        p_blob->total_blob_size = p_blob->base_blob_size;

        strcpy(p_blob->aa_var_names[p_blob->n_vars_in_blob], p_var_name);
        p_blob->a_var_len[p_blob->n_vars_in_blob] = n;
        p_blob->a_var_types[p_blob->n_vars_in_blob] = BLOB_VAR_TYPE_INT;
        p_blob->n_vars_in_blob++;
    }
    else if (  (p_blob->var_idx == p_blob->n_vars_in_blob)
             &&(0 == strcmp(p_var_name, p_blob->aa_var_names[0]))
             )
    {
        unsigned char *p_new_repetition;
        p_new_repetition = (unsigned char*)calloc(p_blob->total_blob_size + p_blob->base_blob_size, 1);
        memcpy(p_new_repetition, p_blob->p_root_blob_data, p_blob->total_blob_size);
        free(p_blob->p_root_blob_data);
        p_blob->p_root_blob_data = p_new_repetition;
        p_blob->p_blob_data = p_blob->p_root_blob_data + p_blob->total_blob_size;
        p_blob->total_blob_size += p_blob->base_blob_size;
        p_blob->n_repetitions++;
    }
    
    /* If we are over the end, point back to the start */
    p_blob->var_idx = p_blob->var_idx % p_blob->n_vars_in_blob; 

    /* Don't suddenly change the variable order! Blob requires this to be constant. */
    if (   (0 != strcmp(p_var_name, p_blob->aa_var_names[p_blob->var_idx]))
        || (BLOB_VAR_TYPE_INT != p_blob->a_var_types[p_blob->var_idx])
        || (n != p_blob->a_var_len[p_blob->var_idx])
        )
    {
        /* Unexpected variable write operation */
        return BLOB_ERR;
    }

    p_var_data = &p_blob->p_blob_data[p_blob->a_var_data_offsets[p_blob->var_idx]];
    for (int i=0; i<n; i++)
    {
        *((int32_t*)&p_var_data[i*sizeof(int32_t)]) = (int32_t)p_var_val[i];
    }
    p_blob->var_idx++;
    return BLOB_OK;
}

int
blob_core_unsigned_int_a(blob_core *p_blob, const char *p_var_name, unsigned int *p_var_val, int n)
{
    unsigned char *p_var_data;

    if (   (p_blob->var_idx == p_blob->n_vars_in_blob)
        && (0 != strcmp(p_var_name, p_blob->aa_var_names[0]))
        )
    {
        unsigned char *p_new_blob;

        /* This occurs only when we are building the first repetition */
        assert(p_blob->n_repetitions == 0);

        /* The variable is new */
        if ((p_blob->n_vars_in_blob + 1) > BLOB_MAX_VARS_PER_BLOB)
        {
            /* Exceeded max size. */
            return BLOB_ERR;
        }

        /* Allocate new, larger blob, copy old blob into it. */
        p_new_blob = (unsigned char*)calloc(p_blob->base_blob_size + sizeof(u_int32_t)*n, 1);
        memcpy(p_new_blob, p_blob->p_blob_data, p_blob->base_blob_size);
        p_blob->a_var_data_offsets[p_blob->n_vars_in_blob] = p_blob->base_blob_size;
        free(p_blob->p_blob_data);
        p_blob->p_blob_data = p_new_blob;
        p_blob->p_root_blob_data = p_new_blob;
        p_blob->base_blob_size += n * sizeof(u_int32_t);
        p_blob->total_blob_size = p_blob->base_blob_size;

        strcpy(p_blob->aa_var_names[p_blob->n_vars_in_blob], p_var_name);
        p_blob->a_var_len[p_blob->n_vars_in_blob] = n;
        p_blob->a_var_types[p_blob->n_vars_in_blob] = BLOB_VAR_TYPE_UNSIGNED_INT;
        p_blob->n_vars_in_blob++;
    }
    else if (  (p_blob->var_idx == p_blob->n_vars_in_blob)
             &&(0 == strcmp(p_var_name, p_blob->aa_var_names[0]))
             )
    {
        unsigned char *p_new_repetition;
        p_new_repetition = (unsigned char*)calloc(p_blob->total_blob_size + p_blob->base_blob_size, 1);
        memcpy(p_new_repetition, p_blob->p_root_blob_data, p_blob->total_blob_size);
        free(p_blob->p_root_blob_data);
        p_blob->p_root_blob_data = p_new_repetition;
        p_blob->p_blob_data = p_blob->p_root_blob_data + p_blob->total_blob_size;
        p_blob->total_blob_size += p_blob->base_blob_size;
        p_blob->n_repetitions++;
    }
    
    /* If we are over the end, point back to the start */
    p_blob->var_idx = p_blob->var_idx % p_blob->n_vars_in_blob; 

    if (   (0 != strcmp(p_var_name, p_blob->aa_var_names[p_blob->var_idx]))
        || (BLOB_VAR_TYPE_UNSIGNED_INT != p_blob->a_var_types[p_blob->var_idx])
        || (n != p_blob->a_var_len[p_blob->var_idx])
        )
    {
        /* Unexpected variable write operation */
        return BLOB_ERR;
    }

    p_var_data = &p_blob->p_blob_data[p_blob->a_var_data_offsets[p_blob->var_idx]];
    for (int i=0; i<n; i++)
    {
        *((u_int32_t*)&p_var_data[i*sizeof(u_int32_t)]) = p_var_val[i];
    }
    p_blob->var_idx++;
    return BLOB_OK;
}

void
blob_core_get_data(blob_core *p_blob, unsigned char **pp_data, size_t *base_blob_size)
{
    *pp_data = p_blob->p_root_blob_data;
    *base_blob_size = p_blob->total_blob_size;
}

void
blob_core_get_info(blob_core *p_blob,
                   int  **pp_var_len,
                   int  **pp_var_types,
                   char **pp_var_names,
                   int  *n_vars,
                   int  *n_repetitions)
{
    *pp_var_len = p_blob->a_var_len;
    *pp_var_types = p_blob->a_var_types;
    *pp_var_names = (char*)p_blob->aa_var_names;
    *n_vars = p_blob->n_vars_in_blob;
    *n_repetitions = p_blob->n_repetitions;
}

size_t
blob_core_set_from_data(blob_core *p_blob,
                        unsigned char *p_data,
                        size_t *p_total_size)
{
    size_t total_size = 0;
    unsigned int offset = 0;
    int i;
    memcpy(&p_blob->n_repetitions, p_data + total_size, sizeof(int));
    total_size += sizeof(int);

    memcpy(&p_blob->n_vars_in_blob, p_data + total_size, sizeof(int));
    total_size += sizeof(int);

    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {
        memcpy(p_blob->aa_var_names[i], p_data + total_size, sizeof(char) * BLOB_MAX_VAR_NAME_LEN);
        total_size += sizeof(char) * BLOB_MAX_VAR_NAME_LEN;
    }
    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {
        memcpy(&p_blob->a_var_types[i], p_data + total_size, sizeof(int));
        total_size += sizeof(int);
    }
    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {
        memcpy(&p_blob->a_var_len[i], p_data + total_size, sizeof(int));
        total_size += sizeof(int);
    }
    p_blob->p_root_blob_data = p_data + total_size;

    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {   
        p_blob->a_var_data_offsets[i] = offset;
        if (p_blob->a_var_types[i] == BLOB_VAR_TYPE_FLOAT)
        {
            offset += (sizeof(float) * p_blob->a_var_len[i]);
        }
        else if (p_blob->a_var_types[i] == BLOB_VAR_TYPE_INT)
        {
            offset += (sizeof(int) * p_blob->a_var_len[i]);
        }
        else if (p_blob->a_var_types[i] == BLOB_VAR_TYPE_UNSIGNED_INT)
        {
            offset += (sizeof(u_int32_t) * p_blob->a_var_len[i]);
        }
    }
    p_blob->base_blob_size = offset;
    p_blob->total_blob_size = offset * (p_blob->n_repetitions + 1);
    total_size += p_blob->total_blob_size;
    *p_total_size = total_size;
    p_blob->serialized_data_size = total_size;
    return 0;
}

size_t
blob_core_get_serialized_data_size(blob_core *p_blob)
{
    return p_blob->serialized_data_size;
}

void
blob_core_update_root_data(blob_core *p_blob, unsigned char *p_data)
{
    p_blob->p_root_blob_data = p_data + 2 * sizeof(int) + p_blob->n_vars_in_blob * (sizeof(char) * BLOB_MAX_VAR_NAME_LEN + sizeof(int) + sizeof(int));
}

int
blob_core_retrieve_float_a(blob_core *p_blob, const char *var_name, const float **pp_var_val, int *p_n, int rep)
{
    int i;
    int var_idx = -1;
    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {
        if (  (0 == strcmp(var_name, p_blob->aa_var_names[i]))
            &&(p_blob->a_var_types[i] == BLOB_VAR_TYPE_FLOAT)
            )
        {
            var_idx = i;
        }
    }
    if (-1 == var_idx)
    {
        printf("Error, no variable with this name found in blob. Likely blob has changed structure.\n");
        return -1;
    }
    *pp_var_val = (const float*)(p_blob->p_root_blob_data + rep * p_blob->base_blob_size + p_blob->a_var_data_offsets[var_idx]);
    *p_n = p_blob->a_var_len[var_idx];
    return BLOB_OK;
}

int
blob_core_retrieve_int_a(blob_core *p_blob, const char *var_name, const int **pp_var_val, int *p_n, int rep)
{
    int i;
    int var_idx = -1;
    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {
        if (  (0 == strcmp(var_name, p_blob->aa_var_names[i]))
            &&(p_blob->a_var_types[i] == BLOB_VAR_TYPE_INT)
            )
        {
            var_idx = i;
        }
    }
    if (-1 == var_idx)
    {
        printf("Error, no variable with this name found in blob. Likely blob has changed structure.\n");
        return -1;
    }
    *pp_var_val = (const int*)(p_blob->p_root_blob_data + rep * p_blob->base_blob_size + p_blob->a_var_data_offsets[var_idx]);
    *p_n = p_blob->a_var_len[var_idx];
    return BLOB_OK;
}

int
blob_core_retrieve_unsigned_int_a(blob_core *p_blob, const char *var_name, const unsigned int **pp_var_val, int *p_n, int rep)
{
    int i;
    int var_idx = -1;
    for (i=0; i<p_blob->n_vars_in_blob; i++)
    {
        if (  (0 == strcmp(var_name, p_blob->aa_var_names[i]))
            &&(p_blob->a_var_types[i] == BLOB_VAR_TYPE_UNSIGNED_INT)
            )
        {
            var_idx = i;
        }
    }
    if (-1 == var_idx)
    {
        printf("Error, no variable with this name found in blob. Likely blob has changed structure.\n");
        return -1;
    }
    *pp_var_val = (const unsigned int*)(p_blob->p_root_blob_data + rep * p_blob->base_blob_size + p_blob->a_var_data_offsets[var_idx]);
    *p_n = p_blob->a_var_len[var_idx];
    return BLOB_OK;
}

int
blob_core_header_get_size(blob_core *p_blob, size_t *p_size)
{
    int *p_var_len;
    int *p_var_types;
    char *p_var_names;
    int n_vars;
    int n_repetitions;
    size_t total_size = 0;

    /* All blobs are the same, just with different data */
    blob_core_get_info(p_blob,
                  &p_var_len,
                  &p_var_types,
                  &p_var_names,
                  &n_vars,
                  &n_repetitions);

    total_size += n_vars * sizeof(int); /* Store the length of each variable */
    total_size += n_vars * sizeof(int); /* Store the types of each variable */
    total_size += n_vars * sizeof(char) * BLOB_MAX_VAR_NAME_LEN; /* Store the variable names */
    total_size += sizeof(int); /* Store the number of variables in the blob */
    total_size += sizeof(int); /* Store the number of repetititions of the variables in the blob */
    *p_size = total_size;
    return BLOB_OK;
}
