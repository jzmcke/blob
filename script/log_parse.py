import pickle
import os
import blob_read as br
import xarray as xr
import csv
import blob_read as br
import numpy as np

log_dict = dict()
def parse_log(log_dir):
    files = [f for f in os.listdir(log_dir) if f.startswith('log_') and f.endswith('.pkl')]
    for f in files:
        with open(os.path.join(log_dir, f), 'rb') as pkl_rd:
            data = pickle.load(pkl_rd)
        
        for i, blob in enumerate(data['data']):
            ip_addr = blob[:128].decode("utf-8").strip("\x00")
            if not (ip_addr in log_dict.keys()):
                log_dict[ip_addr] = []
            
            blob_deserialized, _ = br.blob_read_node_tree(blob[128:])

            blob_unpacked = br.blob_minimal(blob_deserialized)

            log_dict[ip_addr].append({'data': blob_unpacked, 'time': data['times'][i]})

    with open(os.path.join(log_dir, 'parsed.pkl'), 'wb') as pkl_wt:
        pickle.dump(log_dict, pkl_wt)

    return log_dict

def to_csv(log_dict, log_dir):
    for ip_addr in log_dict.keys():
        proto_blob = br.blob_flatten(log_dict[ip_addr][0]['data'])
        for key in proto_blob.keys():
            assert proto_blob[key].shape[0] == 1 and proto_blob[key].shape[1] == 1, f"For csv export compatibility, all data must be 1 dimensional. I.e. no array data and no repetitions. key {key} on {ip_addr} violates this."
        this_csv_vars = proto_blob.keys()
        with open(os.path.join(log_dir, f'{ip_addr}.csv'), 'w', newline='') as f_wt:
            csv_wt = csv.writer(f_wt)
            csv_wt.writerow(["time_s"] + list(this_csv_vars))
            for blob in log_dict[ip_addr]:
                flat_data = br.blob_flatten(blob['data'])
                time = blob['time']
                csv_wt.writerow([time] + [v[0][0] for k,v in flat_data.items()])


def to_nc(log_dict, log_dir):
    for ip_addr in log_dict.keys():
        proto_blob = br.blob_flatten(log_dict[ip_addr][0]['data'])
        arr_dict = dict((k, []) for k in proto_blob.keys())
        time = []
        for blob in log_dict[ip_addr]:
            flat_blob = br.blob_flatten(blob['data'])

            for k in proto_blob.keys():
                arr_dict[k].append(flat_blob[k])
            
            time.append(blob['time'])
        
        time = np.array(time)
        ncdf_dict = dict()
        
        for k in proto_blob.keys():
            vars = np.stack(arr_dict[k])
            ncdf_dict[k] = xr.DataArray(vars, dims=('time', f'n{vars.shape[1]}', 'rep'), coords={'time': time})

        ds = xr.Dataset(ncdf_dict)
        ds.to_netcdf(os.path.join(log_dir, f'{ip_addr}.nc'))

def console():
    import argparse
    parser = argparse.ArgumentParser(description="Log to unpack")
    parser.add_argument("log_dir", help="Path to log directory")
    parser.add_argument("--csv", action="store_true", help="Output csv file")
    parser.add_argument("--nc", action="store_true", help="Output netcdf file")
    opts = parser.parse_args()

    log_dict = parse_log(opts.log_dir)
    
    if opts.csv:
        to_csv(log_dict, opts.log_dir)
    if opts.nc:
        to_nc(log_dict, opts.log_dir)


if __name__ =='__main__':
    console()