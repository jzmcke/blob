import socket
import asyncio
import threading
from blob.blob_cffi import BlobJBUF, BlobFrag

class BlobUDPTx:
    def __init__(self,
                 dest_ip,
                 port=3456,
                 fragment_tx_size=None):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.dest_ip = dest_ip
        self.port = port
        self.fragment_tx_size = fragment_tx_size
        
        self.frag = BlobFrag(1400)

    def send(self, data):
        fragments = self.frag.fragment(data)
        for f in fragments:
            self.sock.sendto(f, (self.dest_ip, self.port))
            print('Sent frag size: ' + str(len(f)))


class BlobUDPRx:
    def __init__(self,
                 port=3456,
                 max_rx_buf_size=4096,
                 rx_jitter_buffer_size=8,
                 rx_callback=None):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.port = port

        # For receiving
        self.max_rx_buf_size = max_rx_buf_size
        
        if rx_callback is not None:
            self.sock.bind(('', port))
            self.stop_event = threading.Event()
            self.rx_callback = rx_callback
        
        self.jbuf = BlobJBUF(rx_jitter_buffer_size, b_tickless=True)
    
    def start(self):
        self.stop_event().clear()
        threading.Thread(target=self.receive_thread).start()

    def receive_thread(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.bind((self.recv_ip_addr, self.port))
            while not self.stop_event.is_set():
                data, addr = sock.recvfrom(self.max_rx_buf_size)  # Adjust the buffer size as needed
                # Strip out the ip address
                res = self.jbuf.push(data)
                res, data = self.jbuf.pull()
                
                if data is not None:
                    self.rx_callback(data, addr)
