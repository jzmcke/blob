import numpy as np
import blob.blob_write as bw
import blob.blob_udp as bu
import sys
bwrite = bw.BlobWriter('test', ['jval', 'jval_squared', 'jval_cubed'])

budp = bu.BlobUDPTx(str(sys.argv[1]), port=int(sys.argv[2]), fragment_tx_size=1400)
i = 1


while (True):
    bwrite.jval = np.array([i])
    bwrite.jval_squared = np.array([i**2])
    bwrite.jval_cubed = np.array([i**3])
    data = bwrite.flush()
    budp.send(data)
