from cffi import FFI
import os
import platform

import copy

class BlobFrag:
    def __init__(self, frag_size):
        self.ffi = FFI()
        self.ffi.cdef("""
            typedef struct blob_frag_tx_s blob_frag_tx;

            int
            blob_frag_tx_init(blob_frag_tx **pp_blob_frag_tx, size_t frag_size);

            int
            blob_frag_tx_begin_packet(blob_frag_tx *p_blob_frag_tx, unsigned char *p_data, size_t n);

            int
            blob_frag_tx_next_packet(blob_frag_tx *p_blob_frag_tx, unsigned char **pp_data, size_t *p_n);
        """)
        if platform.system() == "Windows":
            self.lib = self.ffi.dlopen(os.path.join(os.path.dirname(__file__), r"..", "bin", "blob_jbuf_lib.dll"))
        elif platform.system() == "Linux":
            self.lib = self.ffi.dlopen(os.path.join(os.path.dirname(__file__), r"..", "bin", "libblob_jbuf_lib.so"))
        
        self.frag = self.ffi.new("blob_frag_tx **")
        res = self.lib.blob_frag_tx_init(self.frag, frag_size)
    
    def fragment(self, data):
        send_frags = []
        
        self.lib.blob_frag_tx_begin_packet(self.frag[0], bytes(data), len(data))
        p_out = self.ffi.new("unsigned char **")
        p_n = self.ffi.new("size_t *")

        while True:
            res = self.lib.blob_frag_tx_next_packet(self.frag[0], p_out, p_n)
            if p_out[0] == self.ffi.NULL:
                break
            else:
                send_frags.append(bytes(self.ffi.buffer(p_out[0], p_n[0])))

        return send_frags

class BlobJBUF:
    BLOB_JBUF_TICKLESS_NO_DATA = 5
    def __init__(self, jbuf_len, b_tickless=False):
        self.ffi = FFI()

        self.ffi.cdef("""
            typedef struct blob_jbuf_s blob_jbuf;
            typedef struct blob_jbuf_cfg_s {
                int jbuf_len;
            } blob_jbuf_cfg;

            int blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg);
            int blob_jbuf_push(blob_jbuf *p_jbuf, void *p_new_data, size_t n);
            int blob_jbuf_get_n_fragments(blob_jbuf *p_jbuf);
            int blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_data, size_t *p_n);
            int blob_jbuf_close(blob_jbuf **pp_jbuf);
        """)

        # Load the shared library
        if platform.system() == "Windows":
            self.lib = self.ffi.dlopen(os.path.join(os.path.dirname(__file__), r"..", "bin", "blob_jbuf_lib.dll"))
        elif platform.system() == "Linux":
            self.lib = self.ffi.dlopen(os.path.join(os.path.dirname(__file__), r"..", "bin", "libblob_jbuf_lib.so"))
        
        self.jbuf = self.ffi.new("blob_jbuf **")
        self.cfg = self.ffi.new("blob_jbuf_cfg *")
        self.cfg.jbuf_len = jbuf_len
        res = self.lib.blob_jbuf_init(self.jbuf, self.cfg)
        
        self.b_tickless = b_tickless
        self.n_pushes = 0
        self.p_data = self.ffi.new("void **")
        self.p_n = self.ffi.new("size_t *")
        self.out = self.ffi.cast("unsigned char **", self.p_data)
    
    def push(self, packet):
        res = self.lib.blob_jbuf_push(self.jbuf[0], packet, len(packet))
        self.n_fragments = self.lib.blob_jbuf_get_n_fragments(self.jbuf[0])
        self.n_pushes = (self.n_pushes + 1) % self.n_fragments
        return res
    
    def pull(self):
        data = None
        if self.n_pushes == 0:
            res = self.lib.blob_jbuf_pull(self.jbuf[0], self.p_data, self.p_n)
            self.out = self.ffi.cast("unsigned char **", self.p_data)
            if bool(self.out[0]):
                data = bytes(self.ffi.buffer(self.out[0], self.p_n[0]))
        else:
            res = BlobJBUF.BLOB_JBUF_TICKLESS_NO_DATA
 
        return res, data

