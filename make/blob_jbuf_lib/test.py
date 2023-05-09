import struct
from blob_cffi import BlobJBUF


fmt = "<4i"
N_FRAGS = 3
N_SEQS = 100
packets = [struct.pack(fmt, seq_num, frag_idx, N_FRAGS, seq_num*100 + frag_idx) for seq_num in range(N_SEQS) for frag_idx in range(N_FRAGS)]
idx = 1

fmt_expected = "<3i"

blob_jbuf = BlobJBUF(8)
out = True
for packet in packets:
    res = blob_jbuf.push(packet)
    if idx % 3 == 0:        
        res, out = blob_jbuf.pull()
        if out is not None:
            print(struct.unpack(fmt_expected, out))
    idx += 1


while out is not None:
    res, out = blob_jbuf.pull()
    if out is not None:
        print(struct.unpack(fmt_expected, out))
