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
    # print(f'Error: {error}')
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



def log_while_event(ipaddr, port, event_func):
    """
    Starts log to a file in a folder when an event starts, and finishes when then event ends.
    Args:
        ipaddr (str): Ip address of the device sending messages
        port (int): Port of the device sending messages
        event_func (bool = event_func(message)): A user-defined event function that returns True when the event occurs,
            False when the event ends, and None when no decision can be made.
    returns:
        folder_name: The name of the folder where the log files are stored.
    """
    global b_stop_ws
    global ws
    global folder_name
    b_prev_event = False
    folder_name = 'log_' + datetime.now().strftime("%m_%d_%Y-%H_%M_%S")
    os.mkdir(folder_name)
    
    def on_message_wrapper(ws, message):
        nonlocal b_prev_event
        b_event = event_func(message)
        if not b_event and b_prev_event:
            close_up_shop(None, None)
        elif b_event:
            on_message(ws, message)
        b_prev_event = b_event
    
    # websocket.enableTrace(True)
    ws = websocket.WebSocketApp("ws://" + ipaddr + ":" + port,
                              on_open=on_open,
                              on_message=on_message_wrapper,
                              on_error=on_error,
                              on_close=on_close)


    ws.run_forever()  # Set dispatcher to automatic reconnection
    
    while True:
        if b_stop_ws:
            ws.close()
            break

    return folder_name


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

