import numpy as np
import pickle
from scipy.io import wavfile
import os

pkl = r'C:\git\website\blob\blob\log_03_09_2026-09_20_25\parsed.pkl'

with open(pkl, 'rb') as rd:
    data = pickle.load(rd)
dev_ip = list(data.keys())[0]
print(f'Device: {dev_ip}')
acq_delta_us = np.array([packet['data']['touch_data']['acq_delta_us'][0][0] for packet in data[dev_ip]])
blocking_time_us = np.array([packet['data']['touch_data']['blocking_us'][0][0] for packet in data[dev_ip]])

print(np.mean(acq_delta_us))
print(np.mean(blocking_time_us))

# Extract and write audio streams for each mic channel
mic_channels = ['mic_l1', 'mic_l2', 'mic_r1', 'mic_r2']
output_dir = os.path.dirname(pkl)

for channel in mic_channels:
    # Concatenate audio samples from all packets for this channel
    audio_samples = []
    for packet in data[dev_ip]:
        if channel not in packet['data']['touch_data']:
            continue
        channel_data = packet['data']['touch_data'][channel]
        if channel_data is not None and len(channel_data) > 0:
            audio_samples.append(channel_data)

    if audio_samples:
        # Concatenate all samples into a single contiguous stream
        contiguous_stream = np.concatenate(audio_samples)
        
        # Write to WAV file (assuming 16-bit PCM and sample rate of 48000 Hz)
        output_file = os.path.join(output_dir, f'{channel}.wav')
        wavfile.write(output_file, 32000, contiguous_stream.astype(np.float32))
        print(f'Wrote {channel}.wav with {len(contiguous_stream)} samples')