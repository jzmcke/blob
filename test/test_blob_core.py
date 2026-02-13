import pytest
import sys
import os

# Add .. to path to import blob from blob/blob/
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../blob')))

try:
    from blob_core_cffi import Blob
except ImportError:
    from blob.blob_core_cffi import Blob # alternative if package structure differs

class TestBlobCore:
    def test_basic_structure(self):
        sent_data = []
        
        def send_cb(data):
            sent_data.append(data)
            return 0
            
        b = Blob(send_callback=send_cb)
        b.start("root")
        b.add_int("test_int", 42)
        b.add_float("test_float", 3.14)
        b.flush()
        
        assert len(sent_data) > 0
        total_bytes = sum(len(d) for d in sent_data)
        print(f"Total sent bytes: {total_bytes}")
        assert total_bytes > 0
        
    def test_loopback(self):
        # 1. Generate data
        sent_data = b""
        def send_cb(data):
            nonlocal sent_data
            sent_data += bytes(data)
            return 0
            
        b = Blob(send_callback=send_cb)
        b.start("root")
        b.add_int("loop_int", 999)
        b.flush()
        
        # 2. Prepare for receive
        # We need to add padding because C library expects it (as discovered in C unit tests)
        padding = b'\x00' * 128
        rcv_buffer = padding + sent_data
        
        def rcv_cb():
            return rcv_buffer
            
        # Re-init blob for receiving (Multi-instance supported)
        b_rcv = Blob(rcv_callback=rcv_cb)
        
        b_rcv.retrieve_start("root")
        val = b_rcv.retrieve_int("loop_int")
        
        assert val is not None
        assert len(val) == 1
        assert val[0] == 999

if __name__ == "__main__":
    sys.exit(pytest.main(["-v", __file__]))
