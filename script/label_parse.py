import xarray as xr
import numpy as np


def console():
    import argparse
    parser = argparse.ArgumentParser(description="Log to unpack")
    parser.add_argument("data_file",  help="Data .nc file")
    parser.add_argument("label_file", help="Label .nc file")
    parser.add_argument("output_file", help="Output .nc file")
    
    opts = parser.parse_args()

    label_ds = xr.open_dataset(opts.label_file)
    data_ds = xr.open_dataset(opts.data_file)
    
    data_time = data_ds.time

    label = []
    next_label_idx = 0
    cur_label = 0
    label_idx = -1
    ref_time = 0
    for dt in data_time:
        if next_label_idx < len(label_ds.time) and dt > label_ds.time[next_label_idx]:
            ref_time = label_ds.time[next_label_idx]
            next_label_idx += 1
            label_idx = label_idx + 1

        if dt > ref_time:
            if label_idx == -1:
                cur_label = 0
            else:
                cur_label = label_ds['main.b_label'].values[label_idx][0][0]

        label.append(cur_label)

    label = np.array(label)
    da_label = xr.DataArray(label, dims=('time',), coords={'time': data_ds.time})
    
    data_ds.update({'b_label': da_label})
    data_ds.to_netcdf(opts.output_file)

if __name__ =='__main__':
    console()