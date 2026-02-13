#ifndef BLOB_WS_WIN_H
#define BLOB_WS_WIN_H

#ifdef _WIN32

#include "blob_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Windows-native WebSocket backend.
 * 
 * @param p_cfg Pointer to the blob_comm_cfg to populate with callbacks.
 * @param addr Server address (e.g., "127.0.0.1" or "localhost").
 * @param port Server port (e.g., 8000).
 * @return int 0 on success, -1 on error.
 */
int blob_ws_win_init(blob_comm_cfg *p_cfg, const char *addr, int port);

/**
 * @brief Terminate the WebSocket backend and free resources.
 * 
 * @param p_cfg Pointer to the blob_comm_cfg containing the backend state.
 * @return int 0 on success.
 */
int blob_ws_win_terminate(blob_comm_cfg *p_cfg);

/**
 * @brief Service the websocket (process IO). Should be called periodically.
 * 
 * @param p_cfg Pointer to the blob_comm_cfg containing the backend state.
 * @return int 0 on success.
 */
int blob_ws_win_service(blob_comm_cfg *p_cfg);

#ifdef __cplusplus
}
#endif

#endif // _WIN32
#endif // BLOB_WS_WIN_H
