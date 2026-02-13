#ifndef BLOB_UDP_H
#define BLOB_UDP_H

#include "blob.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blob_udp_s blob_udp;

/* Initialize a UDP blob component.
   p_cfg: pointer to blob_comm_cfg to be populated with callbacks.
   dest_ip: IP address to send to (can be NULL if only receiving).
   dest_port: Port to send to.
   local_port: Port to bind for receiving (0 for random/ephemeral).
   jbuf_len: Length of jitter buffer (packets).
*/
int blob_udp_init(blob_udp **pp_ctx, blob_comm_cfg *p_cfg, const char *dest_ip, int dest_port, int local_port, int jbuf_len);

/* Poll for incoming packets. Should be called periodically or in a thread. 
   Returns 0 on success, error code otherwise. 
   internally calls recvfrom and blob_jbuf_push */
int blob_udp_poll(blob_udp *p_ctx);

/* Clean up and close sockets */
int blob_udp_close(blob_udp **pp_ctx);

/* Get the number of fragments received per packet (from jbuf) */
int blob_udp_get_n_fragments(blob_udp *p_ctx);

#ifdef __cplusplus
}
#endif

#endif // BLOB_UDP_H
