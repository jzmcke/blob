from blob_udp import BlobUDPTx
from blob_write import BlobWriter
import numpy as np

btx = BlobUDPTx('192.168.50.115', 3456, 1400)

blob = BlobWriter('main', ['audio_sin', 'audio_cos', 'audio_square', 'audio_sawtooth'])
samp_idx = np.arange(160)
for i in range(100):
    blob.audio_sin = np.sin(samp_idx * 2 * np.pi * 330 / 16000)
    blob.audio_cos = np.cos(samp_idx * 2 * np.pi * 330 / 16000)
    
    blob.audio_square = np.sign(np.sin(samp_idx * 2 * np.pi * 330 / 16000))
    blob.audio_sawtooth = samp_idx / 160
    data = blob.flush()
    btx.send(data)
    samp_idx += 160
    # print(f'{i}: Sent blob size: ' + str(len(data)))

