from cffi import FFI
import os
import platform
import struct

class Blob:
    def __init__(self, send_callback=None, rcv_callback=None):
        self.ffi = FFI()

        # Define the C structures and functions
        self.ffi.cdef("""
            typedef int (*blob_send_cb_t)(void* context, unsigned char* data, size_t size);
            typedef int (*blob_rcv_cb_t)(void* context, unsigned char** data, size_t* size);

            typedef struct blob_comm_cfg_s {
                blob_send_cb_t p_send_cb;
                void*          p_send_context;
                blob_rcv_cb_t  p_rcv_cb;
                void*          p_rcv_context;
            } blob_comm_cfg;
            
            // blob struct is opaque in header, but we need a pointer to it
            typedef struct blob_s blob;
            
            int blob_init(blob **pp_blob, blob_comm_cfg *p_blob_comm_cfg);
            int blob_close(blob **pp_blob);
            int blob_start(blob *p_blob, const char *node_name);
            int blob_int_a(blob *p_blob, const char *var_name, int *p_var_val, int n);
            int blob_float_a(blob *p_blob, const char *var_name, float *p_var_val, int n);
            int blob_flush(blob *p_blob);
            
            int blob_retrieve_start(blob *p_blob, const char* node_name);
            int blob_retrieve_int_a(blob *p_blob, const char *var_name, const int **pp_var_val, int *p_n, int rep);
            int blob_retrieve_float_a(blob *p_blob, const char *var_name, const float **pp_var_val, int *p_n, int rep);
        """)

        # Load the shared library
        base_path = os.path.dirname(__file__)
        # Build path: ../build/bin/Release/blob_core.dll or similar
        # We need to find the DLL.
        # Assuming we are running from blob/blob/ or blob/test/
        # Let's try to locate it relative to this file.
        
        dll_name = "blob_core.dll" if platform.system() == "Windows" else "libblob_core.so"
        # Search common build paths
        search_paths = [
            os.path.join(base_path, "../../blob/build/bin/Release"),
            os.path.join(base_path, "../../blob/build/bin/Debug"),
            os.path.join(base_path, "../bin/Release"), # If installed
             os.path.join(base_path, "../build/bin/Release")
        ]
        
        self.lib = None
        for p in search_paths:
            full_path = os.path.join(p, dll_name)
            if os.path.exists(full_path):
                self.lib = self.ffi.dlopen(full_path)
                break
        
        if self.lib is None:
            raise FileNotFoundError(f"Could not find {dll_name} in {search_paths}")

        # Store callbacks
        self._send_cb = send_callback
        self._rcv_cb = rcv_callback
        
        # Initialize
        self.cfg = self.ffi.new("blob_comm_cfg *")
        
        # Handle for context
        self.handle = self.ffi.new_handle(self)
        
        if send_callback:
            self.p_send_cb = self.ffi.callback("blob_send_cb_t", self._c_send_cb)
            self.cfg.p_send_cb = self.p_send_cb
            self.cfg.p_send_context = self.handle
            
        if rcv_callback:
            self.p_rcv_cb = self.ffi.callback("blob_rcv_cb_t", self._c_rcv_cb)
            self.cfg.p_rcv_cb = self.p_rcv_cb
            self.cfg.p_rcv_context = self.handle

        self._pp_blob = self.ffi.new("blob **")
        res = self.lib.blob_init(self._pp_blob, self.cfg)
        if res != 0:
            raise RuntimeError(f"blob_init failed: {res}")
            
        self.blob_ptr = self._pp_blob[0]

    def close(self):
        if hasattr(self, '_pp_blob') and self._pp_blob[0] != self.ffi.NULL:
            self.lib.blob_close(self._pp_blob)
            self.blob_ptr = self.ffi.NULL

    def __del__(self):
        self.close()

    @staticmethod
    def _c_send_cb(context, data, size):
        self_obj = FFI().from_handle(context)
        if self_obj._send_cb:
            # Copy data to bytes
            py_data = FFI().buffer(data, size)[:]
            return self_obj._send_cb(py_data)
        return -1

    @staticmethod
    def _c_rcv_cb(context, pp_data, p_size):
        self_obj = FFI().from_handle(context)
        if self_obj._rcv_cb:
            data_bytes = self_obj._rcv_cb()
            if data_bytes is None:
                 return -1
            
            # We must provide a buffer that persists? 
            # Or valid until next call?
            # blob library usually uses it immediately.
            # We need to allocate memory accessible by C.
            # We store it in self_obj to keep it alive
            self_obj._rcv_buffer = self_obj.ffi.new("unsigned char[]", data_bytes)
            pp_data[0] = self_obj._rcv_buffer
            p_size[0] = len(data_bytes)
            return 0
        return -1

    def start(self, name):
        return self.lib.blob_start(self.blob_ptr, name.encode('utf-8'))

    def add_int(self, name, val):
        if not isinstance(val, list):
            val = [val]
        c_arr = self.ffi.new("int[]", val)
        return self.lib.blob_int_a(self.blob_ptr, name.encode('utf-8'), c_arr, len(val))

    def add_float(self, name, val):
        if not isinstance(val, list):
            val = [val]
        c_arr = self.ffi.new("float[]", val)
        return self.lib.blob_float_a(self.blob_ptr, name.encode('utf-8'), c_arr, len(val))

    def flush(self):
        return self.lib.blob_flush(self.blob_ptr)
        
    def retrieve_start(self, name):
        return self.lib.blob_retrieve_start(self.blob_ptr, name.encode('utf-8'))

    def retrieve_int(self, name):
        pp_val = self.ffi.new("int **") # const int ** compatible
        p_n = self.ffi.new("int *")
        
        # Cast to const int** to match signature
        c_pp_val = self.ffi.cast("const int **", pp_val)
        
        res = self.lib.blob_retrieve_int_a(self.blob_ptr, name.encode('utf-8'), c_pp_val, p_n, 0)
        if res != 0:
            return None
        
        n = p_n[0]
        # Copy data
        # Dereference pp_val[0] to get int*
        c_int_ptr = pp_val[0]
        result = []
        for i in range(n):
            result.append(c_int_ptr[i])
        return result
