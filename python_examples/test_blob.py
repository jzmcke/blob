import numpy as np
import blob.blob_write as bw
import blob.blob_udp as bu
import sys
import time
bwrite = bw.BlobWriter('test', ['jval', 'jval_squared', 'jval_cubed', 'abs_jval'])

budp = bu.BlobUDPTx(str(sys.argv[1]), port=int(sys.argv[2]), fragment_tx_size=1400)
i = 1

N_ELEM = 100

while (True):
    jval = (i + 1) % N_ELEM - N_ELEM // 2
    bwrite.jval = np.array([jval])
    bwrite.jval_squared = np.array([jval**2]) / N_ELEM
    bwrite.jval_cubed = np.array([jval**3]) / (N_ELEM**2)
    bwrite.abs_jval = np.array([abs(jval)])
    data = bwrite.flush()
    budp.send(data)
    time.sleep(0.02)
    i = i + 1
