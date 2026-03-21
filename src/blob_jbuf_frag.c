#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob_jbuf_frag.h"
#include "packet.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

#define STATS_BUFFER_SIZE 2000
#define TRACE_BUFFER_SIZE 10000

struct blob_jbuf_s
{
    packet *p_packets; /* blob_jbuf can buffer any data type */
    int          push_idx;
    int          pull_idx;
    int          jbuf_len;
    int          buffer_fullness;
    int          latency;
    int          last_pushed_seq;
    int          b_exit_emptiness;
    int          b_exit_fullness;
    packet_cfg   packet_cfg;
    int          n_fragments;

#ifdef BLOB_JBUF_STATS
    // Statistics (Packet Level)
    char        *identifier;
    long long    last_recv_time;
    long long    perf_freq;
    long long   *recv_deltas;     // Circular buffer of microseconds
    int         *fragment_counts; // Circular buffer of fragments per packet
    int          stats_idx;
    int          stats_count;

    // Full Network Trace (Fragment Level)
    int        *trace_seq;
    int        *trace_frag;
    long long  *trace_time;      // Time since start in microseconds
    int        *trace_status;
    int         trace_idx;
    int         trace_count;
    long long   start_time;
#endif

#ifdef EMSCRIPTEN
    // WASM is single-threaded, no mutex needed
#elif defined(_WIN32)
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
};

int
blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg)
{
    blob_jbuf *p_jbuf;
    p_jbuf = (blob_jbuf*)calloc(1, sizeof(blob_jbuf));
    p_jbuf->p_packets = (packet*)calloc(p_jbuf_cfg->jbuf_len, sizeof(packet));
    p_jbuf->buffer_fullness = 0;
    p_jbuf->b_exit_emptiness = 1;
    p_jbuf->b_exit_fullness = 0;
    p_jbuf->jbuf_len = p_jbuf_cfg->jbuf_len;
    // Decouple latency from total length
    p_jbuf->latency = p_jbuf_cfg->target_latency > 0 ? p_jbuf_cfg->target_latency : p_jbuf->jbuf_len / 2;
    p_jbuf->n_fragments = 1;
    p_jbuf->last_pushed_seq = -1;

    p_jbuf->packet_cfg.deallocate_callback = p_jbuf_cfg->deallocate_callback;
    p_jbuf->packet_cfg.p_context = p_jbuf_cfg->p_context;
    p_jbuf->packet_cfg.total_fragments = 1;
    if (p_jbuf_cfg->jbuf_len < 2)
    {
        blob_jbuf_close(&p_jbuf);
        printf("Error, jbuf_len must be greater than 2.\n");
        return BLOB_JBUF_ERR;
    }

    for (int i=0; i<p_jbuf_cfg->jbuf_len; i++)
    {
        packet_init(&p_jbuf->p_packets[i], &p_jbuf->packet_cfg);
    }
    p_jbuf->push_idx = 0;
    p_jbuf->pull_idx = p_jbuf->jbuf_len - 1;

#ifdef BLOB_JBUF_STATS
    // Initialize Statistics
    p_jbuf->identifier = p_jbuf_cfg->identifier ? _strdup(p_jbuf_cfg->identifier) : _strdup("unknown");
    p_jbuf->recv_deltas = (long long*)calloc(STATS_BUFFER_SIZE, sizeof(long long));
    p_jbuf->fragment_counts = (int*)calloc(STATS_BUFFER_SIZE, sizeof(int));
    p_jbuf->stats_idx = 0;
    p_jbuf->stats_count = 0;
    p_jbuf->last_recv_time = 0;

    // Initialize Trace
    p_jbuf->trace_seq = (int*)calloc(TRACE_BUFFER_SIZE, sizeof(int));
    p_jbuf->trace_frag = (int*)calloc(TRACE_BUFFER_SIZE, sizeof(int));
    p_jbuf->trace_time = (long long*)calloc(TRACE_BUFFER_SIZE, sizeof(long long));
    p_jbuf->trace_status = (int*)calloc(TRACE_BUFFER_SIZE, sizeof(int));
    p_jbuf->trace_idx = 0;
    p_jbuf->trace_count = 0;
    p_jbuf->start_time = 0;

#ifdef _WIN32
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    p_jbuf->perf_freq = freq.QuadPart;
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    p_jbuf->start_time = li.QuadPart;
#else
    p_jbuf->perf_freq = 1000000000LL; // Nanoseconds for clock_gettime
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    p_jbuf->start_time = ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
#endif

#ifdef EMSCRIPTEN
    // No mutex needed for single-threaded WASM
#elif defined(_WIN32)
    InitializeCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_init(&p_jbuf->mutex, NULL);
#endif

    printf("jbuf_len: %d\n", p_jbuf->jbuf_len);
    printf("jbuf_latency: %d\n", p_jbuf->latency);
    *pp_jbuf = p_jbuf;
    return 0;
}


/* Push an element onto the buffer. Push index will never be incremented over the pull.
   Anything that makes it into the queue stays in the queue until it is pulled by the caller
   process and freed. blob_jbuf will never free an object already in the queue itself, but
   will drop packets before it has been added to the queue. */
int
blob_jbuf_push(blob_jbuf *p_jbuf, void *p_new_data, size_t n)
{
    int ret = BLOB_JBUF_OK;

#ifdef EMSCRIPTEN
    // No mutex needed for single-threaded WASM
#elif defined(_WIN32)
    EnterCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_lock(&p_jbuf->mutex);
#endif

    int *p_new_data_ints = (int *)p_new_data;

    // Could add these as input arguments to the function itself.
    int seq_num = p_new_data_ints[0];
    int frag_idx = p_new_data_ints[1];
    int total_fragments = p_new_data_ints[2];

    // --- Sequence Jump Detection / Resync ---
    if (p_jbuf->last_pushed_seq != -1) {
        int seq_diff = seq_num - p_jbuf->last_pushed_seq;
        if (abs(seq_diff) > p_jbuf->jbuf_len) {
            printf("[JBUF] Massive Sequence JUMP detected (%d -> %d). Resyncing...\n", p_jbuf->last_pushed_seq, seq_num);
            for (int i=0; i<p_jbuf->jbuf_len; i++) {
                packet_reset(&p_jbuf->p_packets[i], &p_jbuf->packet_cfg);
            }
            p_jbuf->buffer_fullness = 0;
            p_jbuf->b_exit_emptiness = 1;
            p_jbuf->pull_idx = (seq_num % p_jbuf->jbuf_len + p_jbuf->jbuf_len - 1) % p_jbuf->jbuf_len;
        }
    }
    p_jbuf->last_pushed_seq = seq_num;

#ifdef BLOB_JBUF_STATS
    // --- Statistics Recording ---
    long long now;
#ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    now = li.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif

    if (p_jbuf->last_recv_time > 0) {
        long long delta;
        if (p_jbuf->perf_freq == 1000000000LL) { // Linux/POSIX
            delta = (now - p_jbuf->last_recv_time) / 1000; // Nanoseconds to microseconds
        } else { // Windows
            delta = (now - p_jbuf->last_recv_time) * 1000000 / p_jbuf->perf_freq; // Ticks to microseconds
        }
        
        p_jbuf->recv_deltas[p_jbuf->stats_idx] = delta;
        p_jbuf->fragment_counts[p_jbuf->stats_idx] = total_fragments;
        p_jbuf->stats_idx = (p_jbuf->stats_idx + 1) % STATS_BUFFER_SIZE;
        if (p_jbuf->stats_count < STATS_BUFFER_SIZE) p_jbuf->stats_count++;
    }
    p_jbuf->last_recv_time = now;
    // ---------------------------
#endif

    unsigned char *p_fragment_data = (unsigned char *)&p_new_data_ints[3];
    packet *p_packet = NULL;

    // Push the new fragment into the buffer queue
    p_jbuf->push_idx = seq_num % p_jbuf->jbuf_len;
    //printf("PUSH: Seq %d -> Idx %d. Fullness %d. ExitEmpty %d\n", seq_num, p_jbuf->push_idx, p_jbuf->buffer_fullness, p_jbuf->b_exit_emptiness);

    p_packet = &p_jbuf->p_packets[p_jbuf->push_idx];

    // This line need to be here, since the number of fragments is not known until the first fragment is received.
    if (p_packet->total_fragments != total_fragments)
    {
        if (packet_is_full(p_packet))
        {
            p_jbuf->buffer_fullness--;
        }
        p_jbuf->n_fragments = total_fragments > p_jbuf->n_fragments ? total_fragments : p_jbuf->n_fragments;
        p_jbuf->packet_cfg.total_fragments = total_fragments;
        packet_reset(p_packet, &p_jbuf->packet_cfg);
    }

    if (packet_is_full(p_packet))
    {
        p_jbuf->packet_cfg.total_fragments = total_fragments;
        packet_reset(p_packet, &p_jbuf->packet_cfg);
        p_jbuf->buffer_fullness--;
        ret = BLOB_JBUF_DROPPED_PACKET;
    }
    // Wrap-around safe comparison for "is this packet newer than the one in the slot?"
    int existing_seq = p_packet->seq_num;
    if (existing_seq != -1 && (seq_num - existing_seq) > 0)
    {
        p_jbuf->packet_cfg.total_fragments = total_fragments;
        packet_reset(p_packet, &p_jbuf->packet_cfg);
        printf("New packet has overwritten a packet at idx %d! Calling blob_jbuf_pull too slowly?\n", p_jbuf->push_idx);
        ret = BLOB_JBUF_OVERWROTE_PACKET;
    }

    if (existing_seq != -1 && (seq_num - existing_seq) < 0)
    {
        // This is an old packet arriving late, ignore it.
        ret = BLOB_JBUF_DROPPED_PACKET;
        goto unlock_and_return;
    }

    packet_set_seq_num(p_packet, seq_num);
    
    /* Only add a fragment if the buffer is not attempting latency recovery, the sequence numbers are a match, and there is no data already in the buffer.*/
    if (   (packet_is_fragment_empty(p_packet, frag_idx))
        && (!p_jbuf->b_exit_fullness))
    {
        packet_add_fragment_data(p_packet, p_new_data, n);
    }
    else
    {
        printf("Dropped fragment at idx %d!\n", p_jbuf->push_idx);
        ret = BLOB_JBUF_DROPPED_FRAGMENT;
    }
    
    if (packet_is_full(p_packet))
    {
        p_jbuf->buffer_fullness++;
        if (p_jbuf->buffer_fullness >= p_jbuf->latency)
        {
            p_jbuf->b_exit_emptiness = 0;
        }
        if (p_jbuf->buffer_fullness > p_jbuf->jbuf_len)
        {
            /* This condition should never arise, because we should never push when the buffer is already full. */     
            ret = BLOB_JBUF_ERR;
        }
    }
    else
    {
        // Return JBUF_FRAGMENT_RECEIVED only if there as been no error so far.
        ret = ret == BLOB_JBUF_OK ? BLOB_JBUF_FRAGMENT_RECEIVED : ret;
    }
    
unlock_and_return:
#ifdef BLOB_JBUF_STATS
    // Record fragment-level trace
    {
        long long current_time_us;
#ifdef _WIN32
        LARGE_INTEGER li_trace;
        QueryPerformanceCounter(&li_trace);
        current_time_us = (li_trace.QuadPart - p_jbuf->start_time) * 1000000 / p_jbuf->perf_freq;
#else
        struct timespec ts_trace;
        clock_gettime(CLOCK_MONOTONIC, &ts_trace);
        long long now_ns = ts_trace.tv_sec * 1000000000LL + ts_trace.tv_nsec;
        current_time_us = (now_ns - p_jbuf->start_time) / 1000;
#endif

        p_jbuf->trace_seq[p_jbuf->trace_idx] = seq_num;
        p_jbuf->trace_frag[p_jbuf->trace_idx] = frag_idx;
        p_jbuf->trace_time[p_jbuf->trace_idx] = current_time_us;
        p_jbuf->trace_status[p_jbuf->trace_idx] = ret;
        p_jbuf->trace_idx = (p_jbuf->trace_idx + 1) % TRACE_BUFFER_SIZE;
        if (p_jbuf->trace_count < TRACE_BUFFER_SIZE) p_jbuf->trace_count++;
    }
#endif

#ifdef EMSCRIPTEN
    // No mutex needed for single-threaded WASM
#elif defined(_WIN32)
    LeaveCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_unlock(&p_jbuf->mutex);
#endif

    return ret;
}

int
blob_jbuf_get_n_fragments(blob_jbuf *p_jbuf)
{
    return p_jbuf->n_fragments;
}


/* Pull an element from the buffer */
int
blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_new_data, size_t *p_n)
{
    int ret = BLOB_JBUF_OK;
    /* This function gets called only once we are confident the latest packet isnt required anymore!
    Assumes that receiver has already pulled the latest packet and used it. */
    *pp_new_data = NULL;
    *p_n = 0;

#ifdef EMSCRIPTEN
    // No mutex needed for single-threaded WASM
#elif defined(_WIN32)
    EnterCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_lock(&p_jbuf->mutex);
#endif

    // printf("PULL_ENTRY: ExitEmpty %d. Fullness %d. PullIdx %d\n", p_jbuf->b_exit_emptiness, p_jbuf->buffer_fullness, p_jbuf->pull_idx);

    if (!p_jbuf->b_exit_emptiness)
    {
        /* This condition is true if the buffer has sufficient packets.
        if the buffer underflows, then b_exit_emptiness should be set false until the buffer
        has reached the latency setting */

        int next_idx = (p_jbuf->pull_idx + 1) % p_jbuf->jbuf_len;
        // printf("PULL: Checking Idx %d. Packet Full? %d.\n", next_idx, packet_is_full(&p_jbuf->p_packets[next_idx]));

        if (packet_is_full(&p_jbuf->p_packets[next_idx]))
        {
            // Only reset the OLD packet and advance if we found a NEW one.
            p_jbuf->packet_cfg.total_fragments =  p_jbuf->p_packets[p_jbuf->pull_idx].total_fragments;
            packet_reset(&p_jbuf->p_packets[p_jbuf->pull_idx], &p_jbuf->packet_cfg);
            
            p_jbuf->pull_idx = next_idx;
            
            p_jbuf->buffer_fullness--;
            // the buffer fullness is only incremented when a full packet has been received.

            if (p_jbuf->buffer_fullness == 0)
            {
                /* We need to wait until enough packets have been pushed onto the buffer to resume playback*/
                p_jbuf->b_exit_emptiness = 1;
            }
            // printf("PULL: Returning Idx %d.\n", p_jbuf->pull_idx);

            *pp_new_data = p_jbuf->p_packets[p_jbuf->pull_idx].p_unfragmented_data;
            *p_n = p_jbuf->p_packets[p_jbuf->pull_idx].unfragmented_size;

#ifdef BLOB_LOG_Pull
            //printf("Blob Jbuf Pull Seq: %d Sz: %d\n", p_jbuf->p_packets[p_jbuf->pull_idx].seq_num, (int)*p_n);
#endif  
            ret = BLOB_JBUF_OK; 
        }
        else if (p_jbuf->buffer_fullness > p_jbuf->latency + 4)
        {
            // We are backed up, but the next packet is incomplete or missing.
            // Better to skip the hole and move on to maintain real-time.
            p_jbuf->packet_cfg.total_fragments = p_jbuf->p_packets[p_jbuf->pull_idx].total_fragments;
            packet_reset(&p_jbuf->p_packets[p_jbuf->pull_idx], &p_jbuf->packet_cfg);
            p_jbuf->pull_idx = next_idx;
            ret = BLOB_JBUF_NEED_MORE_DATA; 
        }
        else
        {
            ret = BLOB_JBUF_NEED_MORE_DATA; // Explicitly signal no data
        }
        if (p_jbuf->buffer_fullness <= p_jbuf->latency)
        {
            p_jbuf->b_exit_fullness = 0;
        }

        if (p_jbuf->buffer_fullness < 0)
        {
            /* This condition should never arise, because we should never pull from the buffer when the buffer is empty. */
            printf("Error, attempted pull when buffer already empty!\n");
            ret = BLOB_JBUF_ERR;
        }
    }
    else
    {
        ret = BLOB_JBUF_NEED_MORE_DATA;
    }
    
#ifdef EMSCRIPTEN
    // No mutex needed for single-threaded WASM
#elif defined(_WIN32)
    LeaveCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_unlock(&p_jbuf->mutex);
#endif

    return ret;
}


int
blob_jbuf_close(blob_jbuf **pp_jbuf)
{
    if (pp_jbuf == NULL || *pp_jbuf == NULL) return 0;
    blob_jbuf *p_jbuf = *pp_jbuf;

    for (int i=0; i<p_jbuf->jbuf_len; i++)
    {
        packet_deep_empty(&p_jbuf->p_packets[i]);
    }

#ifdef EMSCRIPTEN
    // No mutex needed for single-threaded WASM
#elif defined(_WIN32)
    DeleteCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_destroy(&p_jbuf->mutex);
#endif

#ifdef BLOB_JBUF_STATS
    if (p_jbuf->identifier) free(p_jbuf->identifier);
    if (p_jbuf->recv_deltas) free(p_jbuf->recv_deltas);
    if (p_jbuf->fragment_counts) free(p_jbuf->fragment_counts);
    if (p_jbuf->trace_seq) free(p_jbuf->trace_seq);
    if (p_jbuf->trace_frag) free(p_jbuf->trace_frag);
    if (p_jbuf->trace_time) free(p_jbuf->trace_time);
    if (p_jbuf->trace_status) free(p_jbuf->trace_status);
#endif
    free(p_jbuf->p_packets);
    free(p_jbuf);
    *pp_jbuf = NULL;
    return BLOB_JBUF_OK;
}

#ifdef BLOB_JBUF_STATS
int
blob_jbuf_dump_stats(blob_jbuf *p_jbuf)
{
    if (!p_jbuf || (p_jbuf->stats_count == 0 && p_jbuf->trace_count == 0)) return BLOB_JBUF_OK;

#ifdef _WIN32
    EnterCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_lock(&p_jbuf->mutex);
#endif

    // Replace ':' with '_' for filename safety
    char safe_id[256];
    strncpy(safe_id, p_jbuf->identifier, 255);
    safe_id[255] = '\0';
    for (char *p = safe_id; *p; p++) if (*p == ':') *p = '_';

    // 1. Packet Stats
    if (p_jbuf->stats_count > 0) {
        char filename[512];
        sprintf(filename, "stats_%s.csv", safe_id);
        FILE *check = fopen(filename, "r");
        int needs_header = (check == NULL);
        if (check) fclose(check);

        FILE *f = fopen(filename, "a");
        if (f) {
            if (needs_header) fprintf(f, "delta_us,total_fragments\n");
            int start = (p_jbuf->stats_idx - p_jbuf->stats_count + STATS_BUFFER_SIZE) % STATS_BUFFER_SIZE;
            for (int i = 0; i < p_jbuf->stats_count; i++) {
                int idx = (start + i) % STATS_BUFFER_SIZE;
                fprintf(f, "%lld,%d\n", p_jbuf->recv_deltas[idx], p_jbuf->fragment_counts[idx]);
            }
            fclose(f);
        }
        p_jbuf->stats_count = 0;
        p_jbuf->stats_idx = 0;
    }

    // 2. Full Network Trace
    if (p_jbuf->trace_count > 0) {
        char filename[512];
        sprintf(filename, "trace_%s.csv", safe_id);
        FILE *check = fopen(filename, "r");
        int needs_header = (check == NULL);
        if (check) fclose(check);

        FILE *f = fopen(filename, "a");
        if (f) {
            if (needs_header) fprintf(f, "seq,frag,time_us,status\n");
            int start = (p_jbuf->trace_idx - p_jbuf->trace_count + TRACE_BUFFER_SIZE) % TRACE_BUFFER_SIZE;
            for (int i = 0; i < p_jbuf->trace_count; i++) {
                int idx = (start + i) % TRACE_BUFFER_SIZE;
                fprintf(f, "%d,%d,%lld,%d\n", p_jbuf->trace_seq[idx], p_jbuf->trace_frag[idx], p_jbuf->trace_time[idx], p_jbuf->trace_status[idx]);
            }
            fclose(f);
        }
        p_jbuf->trace_count = 0;
        p_jbuf->trace_idx = 0;
    }

#ifdef _WIN32
    LeaveCriticalSection(&p_jbuf->mutex);
#else
    pthread_mutex_unlock(&p_jbuf->mutex);
#endif

    return BLOB_JBUF_OK;
}
#endif
