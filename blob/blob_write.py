import struct

def blob_write_node_tree(node_tree):
    # Initialize empty bytearray for binary data
    binary_data = bytearray()
    
    # Add node name to binary data
    binary_data += node_tree['name'].encode()
    binary_data += b'\x00' * (128 - len(node_tree['name']))
    
    # Add number of child nodes to binary data
    binary_data += struct.pack('<i', len(node_tree.get('nodes', {})))
    # Check if node has a blob of data
    if 'data' in node_tree:
        binary_data += struct.pack('<i', 1) # has blob
        
        # Add number of repetitions and variables
        binary_data += struct.pack('<i', node_tree['data']['n_repetitions'])
        binary_data += struct.pack('<i', node_tree['data']['n_variables'])
        
        # Add variable names and types to binary data
        for var in node_tree['data']['vars']:
            binary_data += var.encode()
            binary_data += b'\x00' * (128 - len(var))
        
        for var in node_tree['data']['vars']:
            binary_data += struct.pack('<i', node_tree['data']['vars'][var]['type'])

        for var in node_tree['data']['vars']:
            binary_data += struct.pack('<i', node_tree['data']['vars'][var]['len'])
        
        # Add variable values to binary data
        for rep in range(node_tree['data']['n_repetitions'] + 1):
            for var in node_tree['data']['vars']:
                if node_tree['data']['vars'][var]['type'] == 0:
                    tok = '<{}i'.format(node_tree['data']['vars'][var]['len'])
                elif node_tree['data']['vars'][var]['type'] == 2:
                    tok = '<{}I'.format(node_tree['data']['vars'][var]['len'])
                elif node_tree['data']['vars'][var]['type'] == 1:
                    tok = '<{}f'.format(node_tree['data']['vars'][var]['len'])
                
                binary_data += struct.pack(tok, *node_tree['data']['vars'][var]['value'][:, rep])
    else:
        binary_data += struct.pack('<i', 0) # no blob
    
    # Recursively add child nodes to binary data
    for child_node in node_tree.get('nodes', {}):
        binary_data += blob_write_node_tree(child_node)
    
    return binary_data

import numpy as np


class BlobWriter:
    def __init__(self, root_name, vars):
        self.dict_tx = {'name': root_name, 'data': {} }
        data = {'n_repetitions': 0, 'n_variables': len(vars), 'vars': {v : { 'type': 0, 'len': 1, 'value': np.array([[0]]).T } for v in vars}}
        self.dict_tx['data'] = data
        
        for v in vars:
            setattr(self, v, np.array([0]))
    
    def __setattr__(self, key, value):
        if key not in {"dict_tx"}:
            if value.dtype == np.int32:
                type = 0
            elif value.dtype == np.uint32:
                type = 2
            else:
                value = value.astype(np.float32)
                type = 1

            self.dict_tx['data']['vars'][key] = {'type': type, 'len': len(value), 'value': np.array([value]).T}
        super().__setattr__(key, value)
        
    
    def flush(self):
        data = blob_write_node_tree(self.dict_tx)
        return data
