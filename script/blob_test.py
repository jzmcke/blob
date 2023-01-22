import blob_write as bw
import blob_read as br
import numpy as np
# Create a simple input dictionary
node_tree = {
    'name': 'node1',
    'data': {
        'n_repetitions': 1,
        'n_variables': 2,
        'vars': {
            'var1': {
                'type': 0,
                'len': 4,
                'value': np.array([[1, 2, 3, 4], [5, 6, 7, 8]]).T
            },
            'var2': {
                'type': 1,
                'len': 2,
                'value': np.array([[1.1, 2.2], [3.3, 4.4]]).T
            }
        }
    },
    'nodes': [
        {
            'name': 'node2',
            'data': {
                'n_repetitions': 0,
                'n_variables': 1,
                'vars': {
                    'var3': {
                        'type': 2,
                        'len': 3,
                        'value': np.array([[1, 2, 3]]).T
                    }
                }
            }
        }
    ]
}

# Serialize the input dictionary
binary_data = bw.blob_write_node_tree(node_tree)

out_tree = br.blob_read_node_tree(binary_data)

print(out_tree)