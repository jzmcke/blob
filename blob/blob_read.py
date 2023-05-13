import struct
import numpy as np

def blob_read_node_tree(data):
    data_remaining = len(data)
    new_data = data
    blob = dict()
        
    # Extract node name
    node_name = ''.join( [chr(c) for c in data[:128] if c != 0])
    new_data = new_data[128:]

    n_children = struct.unpack('<i', new_data[:4])[0]
    new_data = new_data[4:]
    
    b_has_blob = struct.unpack('<i', new_data[:4])[0]
    new_data = new_data[4:]

    if b_has_blob:
        blob['data'] = dict()
        blob['data']['n_repetitions'] = struct.unpack('<i', new_data[:4])[0]
        new_data = new_data[4:]

        blob['data']['n_variables'] = struct.unpack('<i', new_data[:4])[0]
        new_data = new_data[4:]

        blob['data']['vars'] = dict()
        for i in range(blob['data']['n_variables']):
            var_name = ''.join( [chr(c) for c in new_data[:128] if c != 0])
            new_data = new_data[128:]
            blob['data']['vars'][var_name] = dict()
        
        for i, var in enumerate(blob['data']['vars'].keys()):
            blob['data']['vars'][var]['type'] = struct.unpack('<i', new_data[:4])[0]
            new_data = new_data[4:]
        
        for i, var in enumerate(blob['data']['vars'].keys()):
            blob['data']['vars'][var]['len'] = struct.unpack('<i', new_data[:4])[0]
            new_data = new_data[4:]
            blob['data']['vars'][var]['value'] = np.zeros((blob['data']['vars'][var]['len'], blob['data']['n_repetitions'] + 1))

        
        for rep in range(blob['data']['n_repetitions']+1):
            for i, var in enumerate(blob['data']['vars'].keys()):
                if blob['data']['vars'][var]['type'] == 0:
                    tok = '<{}i'.format(blob['data']['vars'][var]['len'])
                elif blob['data']['vars'][var]['type'] == 2:
                    tok = '<{}I'.format(blob['data']['vars'][var]['len'])
                elif blob['data']['vars'][var]['type'] == 1:
                    tok = '<{}f'.format(blob['data']['vars'][var]['len'])
                
                blob['data']['vars'][var]['value'][:, rep] = struct.unpack(tok, new_data[:4*blob['data']['vars'][var]['len']])
                new_data = new_data[4*blob['data']['vars'][var]['len']:]

    if n_children > 0:
        blob['nodes'] = dict()

    for i in range(n_children):
        new_node, new_data = blob_read_node_tree(new_data)
    
    blob['name'] = node_name
    return blob, new_data

def blob_minimal(node_tree):
    out = dict()
    if node_tree['data']['n_variables'] > 0:
        for var in node_tree['data']['vars'].keys():
            out[var] = node_tree['data']['vars'][var]['value']
        
    if 'nodes' in node_tree:
        for node_name in node_tree['nodes'].keys():
            out.update(blob_minimal(node_tree['nodes'][node_name]))

    return {node_tree['name']: out}

def blob_flatten(blob_minimal):
    var_names = []
    var_vals = []
    for key in blob_minimal:
        if isinstance(blob_minimal[key], dict):
            new_dict = blob_flatten(blob_minimal[key])
            newvarnames = new_dict.keys()
            newvarvals = [new_dict[key] for key in newvarnames]
            for varname, varval in zip(newvarnames, newvarvals):
                var_names.append(key + '.' + varname)
                var_vals.append(varval)
        else:
            var_names.append(key)
            var_vals.append(blob_minimal[key])

    return dict((k, v) for k, v in zip(var_names, var_vals))


class BlobReader:
    def read(data):
        node_tree, new_data = blob_read_node_tree(data)
        blob = blob_minimal(node_tree)
        blob = blob_flatten(blob)
        return blob


if __name__ == '__main__':
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument("-f", "--file", dest="filename",
                        help="Read from file", metavar="FILE")
    
    args = parser.parse_args()
    with open(args.filename, 'rb') as rd:
        file_data = rd.read()
        node_tree = dict()
        blob, new_data = blob_read_node_tree(file_data)



