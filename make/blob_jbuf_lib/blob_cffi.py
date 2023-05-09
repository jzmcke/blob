from cffi import FFI
import os

import struct


class BlobJBUF:
    def __init__(self, jbuf_len):
        
        self.ffi = FFI()

        self.ffi.cdef("""
            typedef struct blob_jbuf_s blob_jbuf;
            typedef struct blob_jbuf_cfg_s {
                int jbuf_len;
            } blob_jbuf_cfg;

            int blob_jbuf_init(blob_jbuf **pp_jbuf, blob_jbuf_cfg *p_jbuf_cfg);
            int blob_jbuf_push(blob_jbuf *p_jbuf, void *p_new_data, size_t n);
            int blob_jbuf_pull(blob_jbuf *p_jbuf, void **pp_data, size_t *p_n);
            int blob_jbuf_close(blob_jbuf **pp_jbuf);
        """)

        # Load the shared library
        self.lib = self.ffi.dlopen(os.path.join(os.path.dirname(__file__), r"build\bin\Release\blob_jbuf_lib.dll"))
        
        self.jbuf = self.ffi.new("blob_jbuf **")
        self.cfg = self.ffi.new("blob_jbuf_cfg *")
        self.cfg.jbuf_len = jbuf_len
        res = self.lib.blob_jbuf_init(self.jbuf, self.cfg)
        self.p_data = self.ffi.new("void **")
        self.p_n = self.ffi.new("size_t *")
        self.out = self.ffi.cast("unsigned char **", self.p_data)
    
    def push(self, packet):
        res = self.lib.blob_jbuf_push(self.jbuf[0], packet, len(packet))
        return res
    
    def pull(self):
        res = self.lib.blob_jbuf_pull(self.jbuf[0], self.p_data, self.p_n)
        self.out = self.ffi.cast("unsigned char **", self.p_data)
        if bool(self.out[0]):
            data = bytes(self.ffi.buffer(self.out[0], self.p_n[0]))
        else:
            data = None
        
        return res, data

