from cffi import FFI
import os

import struct


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
        self.lib = self.ffi.dlopen(os.path.join(os.path.dirname(__file__), r"build\bin\Release\blob_jbuf_lib.dll"))
        
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

