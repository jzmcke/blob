#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob_ws_win.h"
#include "blob.h"
#include "wav_exporter.h"

#define SAMPLE_RATE 16000
#define DURATION_S 10
#define TOTAL_SAMPLES (SAMPLE_RATE * DURATION_S)
#define WS_PORT 8000


int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    if (argc > 1) host = argv[1];

    printf("Starting Audio Recorder...\n");
    printf("Connecting to WebSocket server at %s:%d\n", host, WS_PORT);

    blob_comm_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (blob_ws_win_init(&cfg, host, WS_PORT) != 0) {
        fprintf(stderr, "Failed to initialize WebSocket client. Ensure core-server is running on port %d.\n", WS_PORT);
        return 1;
    }

    blob *p_blob = NULL;
    if (blob_init(&p_blob, &cfg) != BLOB_OK) {
        fprintf(stderr, "Failed to initialize blob decoder.\n");
        blob_ws_win_terminate(&cfg);
        return 1;
    }

    float *audio_buffer = malloc(TOTAL_SAMPLES * sizeof(float));
    if (!audio_buffer) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }
    int samples_collected = 0;

    printf("Waiting for audio data (node: 'audio_data', var: 'audio')...\n");
    printf("Need %d samples (%.1f seconds at %dHz)\n", TOTAL_SAMPLES, (float)DURATION_S, SAMPLE_RATE);

    DWORD last_print_time = 0;
    DWORD start_time = GetTickCount();
    
    while (samples_collected < TOTAL_SAMPLES) {
        blob_ws_win_service(&cfg);

        // Attempt to retrieve audio data
        if (blob_retrieve_start(p_blob, "audio_data") == BLOB_OK) {
            const float *p_audio = NULL;
            int n_audio = 0;
            
            if (blob_retrieve_float_a(p_blob, "audio", &p_audio, &n_audio, 0) == BLOB_OK) {
                if (n_audio > 0) {
                    int to_copy = n_audio;
                    if (samples_collected + to_copy > TOTAL_SAMPLES) {
                        to_copy = TOTAL_SAMPLES - samples_collected;
                    }
                    
                    memcpy(audio_buffer + samples_collected, p_audio, to_copy * sizeof(float));
                    samples_collected += to_copy;
                    
                    DWORD now = GetTickCount();
                    if (now - last_print_time > 100) {
                        printf("\rProgress: [%-20s] %d/%d (%d%%)", 
                            "####################", // This will be sliced
                            samples_collected, TOTAL_SAMPLES, 
                            (samples_collected * 100) / TOTAL_SAMPLES);
                        // Simple ASCII progress bar logic
                        int bars = (samples_collected * 20) / TOTAL_SAMPLES;
                        printf("\rProgress: [");
                        for(int i=0; i<20; i++) printf(i < bars ? "#" : " ");
                        printf("] %d/%d (%d%%)", samples_collected, TOTAL_SAMPLES, (samples_collected * 100) / TOTAL_SAMPLES);
                        fflush(stdout);
                        last_print_time = now;
                    }
                }
            }
        }

        Sleep(5); // Don't peg the CPU

        if (GetTickCount() - start_time > 60000) { // 60s timeout
            printf("\nTimeout after 60 seconds. Incomplete recording (%d samples).\n", samples_collected);
            break;
        }
    }

    if (samples_collected > 0) {
        printf("\nRecording complete! Accumulated %d samples.\n", samples_collected);
        printf("Saving to 'recorded_audio.wav'...\n");
        
        if (wav_exporter_save("recorded_audio.wav", audio_buffer, samples_collected, SAMPLE_RATE) == 0) {
            printf("Success! WAV file created.\n");
        } else {
            fprintf(stderr, "Error: Could not save WAV file.\n");
        }
    }

    free(audio_buffer);
    blob_close(&p_blob);
    blob_ws_win_terminate(&cfg);

    printf("Recorder closed.\n");
    return 0;
}
