from cmath import e
import websocket
import rel
import concurrent.futures
import blob_read as br
import time
from datetime import datetime
import pickle
import signal
import sys
import os

b_msg_lock = False
times = []
msgs = []
queue = []
file_count = 0
folder_name = None
start_time = time.time()
log_dict = dict()


b_stop_ws = False
ws = None

def on_message(ws, message):
    global msgs
    global file_count
    global times
    # print('Msg received. len msgs: ' + str(len(msgs)))
    msgs.append(message)
    times.append(time.time() - start_time)

    if len(msgs) > 50000:
        with open(os.path.join(folder_name, f"log_{file_count}.pkl"),'wb') as pkl_wt:
            pickle.dump({'data': msgs, 'times': times}, pkl_wt)
        file_count += 1
        msgs = []
        times = []

def on_error(ws, error):
    # print(error)
    pass

def on_close(ws, close_status_code, close_msg):
    print("### closed ###")
    

def on_open(ws):
    print("Opened connection")

def close_up_shop(signal, frame):
    global ws
    global msgs
    global times
    global b_stop_ws
    print('Logging ' + str(len(msgs)) + ' packets of data')
    with open(os.path.join(folder_name, f"log_{file_count}.pkl"),'wb') as pkl_wt:
        pickle.dump({'data': msgs, 'times': times}, pkl_wt)      
    msgs = []
    
    b_stop_ws = True
    ws.close()

def log_til_sigint(ipaddr, port):
    global b_stop_ws
    global ws
    global folder_name
    folder_name = 'log_' + datetime.now().strftime("%m_%d_%Y-%H_%M_%S")
    os.mkdir(folder_name)

    signal.signal(signal.SIGINT, close_up_shop)
    print('Press CTRL+C')
    
    # websocket.enableTrace(True)
    ws = websocket.WebSocketApp("ws://" + ipaddr + ":" + port,
                              on_open=on_open,
                              on_message=on_message,
                              on_error=on_error,
                              on_close=on_close)


    ws.run_forever()  # Set dispatcher to automatic reconnection
    
    while True:
        if b_stop_ws:
            ws.close()
            break
    
    return folder_name


if __name__ == "__main__":
    log_til_sigint("192.168.50.115", str(8000))

