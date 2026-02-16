#ifndef BLOB_H
#define BLOB_H

#define BLOB_OK          (0)
#define BLOB_ERR         (-1)


#include "blob_comm.h"

typedef struct blob_s blob;


/* Add the relevant define into the main.c/main.cpp, or makefile, of your project. */
// Supported configs: BLOB_ESP32_WEBSOCKETS, BLOB_WEBSOCKETS, BLOB_FILE, BLOB_ESP32_UDP

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

int
blob_retrieve_clear(blob *p_blob);


#ifdef BLOB_ESP32_WEBSOCKETS
    extern int _blob_espws_init(blob_comm_cfg*,const char*, int);
    extern int _blob_espws_terminate(blob_comm_cfg*);
    #define BLOB_INIT(address, port)                                   _blob_espws_init(&g_blob_ccfg, address, port); blob_init(&g_blob_ccfg)
    #define BLOB_TERMINATE()                                           _blob_espws_terminate(&g_blob_ccfg)
#endif


#ifdef BLOB_ESP32_UDP
    extern int _blob_udp_init(blob_comm_cfg*,const char*, int);
    extern int _blob_udp_terminate(blob_comm_cfg*);
    #define BLOB_INIT(address, port)                                   _blob_udp_init(&g_blob_ccfg, address, port); blob_init(&g_blob_ccfg)
    #define BLOB_TERMINATE()                                           _blob_udp_terminate(&g_blob_ccfg)
#endif

#ifdef BLOB_WEBSOCKETS
    extern int _blob_minws_init(blob_comm_cfg*, const char*, int);
    extern int _blob_minws_terminate(blob_comm_cfg*);
    #define BLOB_INIT(address, port)                                   _blob_minws_init(&g_blob_ccfg, address, port); blob_init(&g_blob_ccfg)
    #define BLOB_TERMINATE()                                           _blob_minws_terminate(&g_blob_ccfg)
#endif

#ifdef BLOB_FILE
   #define BLOB_INIT()                                                (void)
   #define BLOB_TERMINATE()                                           (void)
#endif



#endif
