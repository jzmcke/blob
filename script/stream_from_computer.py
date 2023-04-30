import time
import blob_write as bw
import socket
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

import pyaudiowpatch as pyaudio
import time
import numpy as np

node_tree = {}

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

def send_blob(data, tick_ms, device_sample_rate_hz, output_sample_rate):
    if data.dtype == np.int16: 
        data = data.astype(np.float32) / 32768.0
    elif data.dtype == np.int32:
        data = data.astype(np.float32) / 2147483648.0
    elif data.dtype == np.float32:
        data = data.astype(np.float32)
    else:
        assert False, "Currently unsupported data format"
    
    # Sum two chans.
    if len(data.shape) > 1:
        data = np.sum(data, axis=1) / data.shape[1]

    if device_sample_rate_hz != output_sample_rate:
        data = resample_poly(data, device_sample_rate_hz, output_sample_rate)

    n_samples_per_tick = int(device_sample_rate_hz * tick_ms / 1000)
    
    if len(data) != n_samples_per_tick:
        print("Error, data length is not equal to n_samples_per_tick") 
        print("data length: " + str(len(data)))
    else:
        # Increment the integer in the input dictionary
        node_tree['data']['vars']['forward']['value'] = np.array([data]).T.astype(np.float32)

        # Serialize the input dictionary
        binary_data = bw.blob_write_node_tree(node_tree)
        
        sock.sendto(binary_data, (IP_ADDR, UDP_PORT))
        print('Sent blob size: ' + str(len(binary_data)))

if __name__ == '__main__':
    import argparse
    # Get user input for wav file name
    parser = argparse.ArgumentParser(description='Play a wav file.')
    parser.add_argument('tick_ms', type=int, help='Number of milliseconds per transmitted frame.')
    parser.add_argument('device_sample_rate_hz', type=int, help='Sample rate of the device.')
    parser.add_argument('audio_output_rate_hz', type=int, help='Sample rate of the computer device.')
    parser.add_argument('--device_id', type=int, default=23, help='Audio device ID')
    args = parser.parse_args()

    # Connect to the WebSocket server
    IP_ADDR = "192.168.50.115"
    UDP_PORT = 3456

    N = int(args.tick_ms * args.device_sample_rate_hz / 1000)
    init_data = np.zeros(N)
    # Create a simple input dictionary
    node_tree.update({
        'name': 'main',
        'data': {
            'n_repetitions': 0,
            'n_variables': 1,
            'vars': {
                'forward': {
                    'type': 1, # float 1, int 0, uint 2
                    'len': N,
                    'value': np.array([init_data]).T
                }
            }
        }
    })

    # Calculate the RMS value of a numpy array
    def rms(data):
        return np.sqrt(np.mean(np.square(data)))

    # Callback function for the PyAudio stream
    def callback(in_data, frame_count, time_info, status):
        audio_data = np.frombuffer(in_data, dtype=np.int16)
        print(len(audio_data))
        send_blob(audio_data, args.tick_ms, args.device_sample_rate_hz, args.audio_output_rate_hz)
        return (in_data, pyaudio.paContinue)

    # Parameters for the PyAudio stream
    rate = args.audio_output_rate_hz
    channels = 1
    format = pyaudio.paInt16
    frames_per_buffer = int(rate * args.tick_ms / 1000)  # 10ms tick

    # Initialize PyAudio and create a stream
    audio = pyaudio.PyAudio()
    
    stream = audio.open(rate=args.audio_output_rate_hz,
                        channels=channels,
                        format=format,
                        input_device_index=args.device_id,
                        input=True,
                        frames_per_buffer=frames_per_buffer,
                        stream_callback=callback)

    # Start the stream and let it run for 5 seconds
    stream.start_stream()

    # Stop the stream and terminate PyAudio
    import signal
    
    def handler(signum, frame):
        stream.stop_stream()
        stream.close()
        audio.terminate()
    
    signal.signal(signal.SIGINT, handler)
    
    while True:
        time.sleep(0.1)
