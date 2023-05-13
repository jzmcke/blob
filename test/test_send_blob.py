import asyncio
import struct
import time
import blob_write as bw
import socket
import numpy as np
import websockets

LOOP_DELAY_S = 0.01

SAMPLE_RATE_HZ = 16000
TONE_FREQUENCY_HZ = 330
AMP = 0.01
N = int(LOOP_DELAY_S * SAMPLE_RATE_HZ)
i = np.arange(N)

tone = np.sin(2*np.pi * TONE_FREQUENCY_HZ / SAMPLE_RATE_HZ * i)

# Create a simple input dictionary
node_tree = {
    'name': 'main',
    'data': {
        'n_repetitions': 0,
        'n_variables': 1,
        'vars': {
            'forward': {
                'type': 1, # float 1, int 0, uint 2
                'len': N,
                'value': np.array([tone]).T
            }
        }
    }
}

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)


def send_blob():
    # Connect to the WebSocket server
    IP_ADDR = "192.168.50.115"
    WS_PORT = 8000
    UDP_PORT = 3456
    uri = "ws://" + IP_ADDR + ":" + str(WS_PORT)

    while True:

        global i

        start_time = time.perf_counter()
        tone = AMP * np.cos(2*np.pi * TONE_FREQUENCY_HZ / SAMPLE_RATE_HZ * i)
        i += len(i)

        # Increment the integer in the input dictionary
        node_tree['data']['vars']['forward']['value'] = np.array([tone]).T.astype(np.float32)

        # Serialize the input dictionary
        binary_data = bw.blob_write_node_tree(node_tree)
        
        sock.sendto(binary_data, (IP_ADDR, UDP_PORT))
        print('Sent blob size: ' + str(len(binary_data)))
        mid_time = time.perf_counter()

        while True:
            end_time = time.perf_counter()
            if end_time-start_time >= LOOP_DELAY_S:
                break

        # Wait LOOP_DELAY_S milliseconds before sending the next packet
        # time.sleep(LOOP_DELAY_S - (end_time-start_time))
        


# Start sending the blob
send_blob()
