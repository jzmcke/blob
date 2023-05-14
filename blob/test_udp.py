from blob_udp import BlobUDPTx
from blob_write import BlobWriter
import numpy as np

btx = BlobUDPTx('192.168.50.115', 3456, 1400)

blob = BlobWriter('main', ['count0', 'count1'])
for i in range(100):
    blob.count0 = np.array([i])
    blob.count1 = np.array([i]) ** 2
    data = blob.flush()
    btx.send(data)
    print(f'{i}: Sent blob size: ' + str(len(data)))

