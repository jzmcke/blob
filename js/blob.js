
function readInt32(buffer)
{
    dv = new DataView(buffer);
    val = dv.getInt32(0, true);
    buffer = buffer.slice(4);
    return [buffer, val];
}

function readString(buffer)
{
    val = String.fromCharCode.apply(null, new Uint8Array(buffer.slice(0,128))).replace(/\0.*$/g,'');
    buffer = buffer.slice(128);
    return [buffer, val];
}

function writeString(buffer, string, strlen)
{
    var out_buf = new ArrayBuffer(buffer.byteLength + strlen);

    /* First, copy the existing buffer into the new array buffer */
    new Uint8Array(out_buf).set(new Uint8Array(buffer));

    /* Create a view into the buffer with offset of the original buffer length */
    var strview = new Int8Array(out_buf, buffer.byteLength, string.length);
    var i;
    for (i=0; i<string.length; i++)
    {
        strview[i] = string.charCodeAt(i);
    }
    return out_buf;
}

function writeInt32Array(buffer, arr)
{
    var out_buf = new ArrayBuffer(buffer.byteLength + arr.byteLength);

    /* First, copy the existing buffer into the new array buffer */
    new Uint8Array(out_buf).set(new Uint8Array(buffer));
    new Int32Array(out_buf, buffer.byteLength, arr.length).set(new Int32Array(arr));
    return out_buf;
}

function writeFloat32Array(buffer, arr)
{
    var out_buf = new ArrayBuffer(buffer.byteLength + arr.byteLength);

    /* First, copy the existing buffer into the new array buffer */
    new Uint8Array(out_buf).set(new Uint8Array(buffer));
    new Float32Array(out_buf, buffer.byteLength, arr.length).set(new Float32Array(arr));
    return out_buf;
}

function writeUInt32Array(buffer, arr)
{
    var out_buf = new ArrayBuffer(buffer.byteLength + arr.byteLength);

    /* First, copy the existing buffer into the new array buffer */
    new Uint8Array(out_buf).set(new Uint8Array(buffer));
    new Uint32Array(out_buf, buffer.byteLength, arr.length).set(new Uint32Array(arr));
    return out_buf;
}

function blobEncode(in_data, name, buffer)
{   
    buffer = writeString(buffer, name, 128);

    /* This may not be the correct notation */
    var n_entries = Object.keys(in_data).length;
    var keys = Object.keys(in_data);
    var n_children = 0;
    var n_vals = 0;
    var children = [];
    var children_names = [];
    var variables = [];
    var variable_names = [];
    var child_idx, entry_idx, var_idx;
    for (entry_idx=0; entry_idx<n_entries; entry_idx++)
    {
        if (in_data[keys[entry_idx]].constructor === Int32Array || in_data[keys[entry_idx]].constructor === Float32Array || in_data[keys[entry_idx]].constructor === Uint32Array)
        {
            variables.push(in_data[keys[entry_idx]]);
            variable_names.push(keys[entry_idx]);
            n_vals++;
        }
        else
        {
            n_children++;
            children.push(in_data[keys[entry_idx]]);
            children_names.push(keys[entry_idx]);
        }
    }
    /* n_children */
    buffer = writeInt32Array(buffer, new Int32Array([n_children]));

    /* b_has_blob */
    if (n_vals > 0)
    {
        buffer = writeInt32Array(buffer, new Int32Array([1]));
    }
    else
    {
        buffer = writeInt32Array(buffer, new Int32Array([0]));
    }

    if (n_vals > 0)
    {
        /* Only can write nreps = 0 for now */
        buffer = writeInt32Array(buffer, new Int32Array([0]));
        /* n variables */
        buffer = writeInt32Array(buffer, new Int32Array([n_vals]));
        for (var_idx=0; var_idx<n_vals; var_idx++)
        {
            buffer = writeString(buffer, variable_names[var_idx], 128);
        }
        /* type */
        for (var_idx=0; var_idx<n_vals; var_idx++)
        {

            if (variables[var_idx].constructor === Float32Array)
            {
                buffer = writeInt32Array(buffer, new Int32Array([1]));
            }
            if (variables[var_idx].constructor === Int32Array)
            {
                buffer = writeInt32Array(buffer, new Int32Array([0]));
            }
            if (variables[var_idx].constructor === Uint32Array)
            {
                buffer = writeInt32Array(buffer, new Int32Array([2]));
            }
        }
        /* lens */
        for (var_idx=0; var_idx<n_vals; var_idx++)
        {
            buffer = writeInt32Array(buffer, new Int32Array([variables[var_idx].length]));
        }
        for (var_idx=0; var_idx<n_vals; var_idx++)
        {
            var i;
            if (variables[var_idx].constructor === Float32Array)
            {
                
                buffer = writeFloat32Array(buffer, variables[var_idx]);
            }
            if (variables[var_idx].constructor === Int32Array)
            {
                buffer = writeInt32Array(buffer, variables[var_idx]);
            }
            if (variables[var_idx].constructor === Uint32Array)
            {
                buffer = writeUInt32Array(buffer, variables[var_idx]);
            }
        }
    }

    for (child_idx=0; child_idx<n_children; child_idx++)
    {
        buffer = blobEncode(children[child_idx], children_names[child_idx], buffer);
    }
    return buffer;
}

function blobDecode(buffer)
{
    var out = {};
    var nodename;
    var n_children;
    var b_has_blob;
    var n_variables;
    var varnames, varname;
    var types, type;
    var lens, len;

    [buffer, nodename] = readString(buffer);

    [buffer, n_children] = readInt32(buffer);

    [buffer, b_has_blob] = readInt32(buffer);

    if (b_has_blob == 1) {
        [buffer, n_repetitions] = readInt32(buffer);
        [buffer, n_variables] = readInt32(buffer);

        varnames = []
        types = []
        lens = []
        for (i=0; i<n_variables; i++)
        {
            [buffer, varname] = readString(buffer);
            out[varname] = []
            varnames.push(varname);
        }
        for (i=0; i<n_variables; i++)
        {
            [buffer, type] = readInt32(buffer);
            types.push(type);
        }
        for (i=0; i<n_variables; i++)
        {
            [buffer, len] = readInt32(buffer);
            lens.push(len);
        }
        
        for (rep=0; rep<n_repetitions+1; rep++)
        {
            for (i=0; i<n_variables; i++)
            {
                if (types[i] == 0)
                {
                    /* int */
                    val = new Int32Array(buffer.slice(0, lens[i] * 4));
                    buffer = buffer.slice(lens[i] * 4);
                } else if (types[i] == 1)
                {
                    /* float */
                    val = new Float32Array(buffer.slice(0, lens[i] * 4));
                    buffer = buffer.slice(lens[i] * 4);
                } else if (types[i] == 2)
                {
                    /* unsigned int */
                    val = new Uint32Array(buffer.slice(0, lens[i] * 4));
                    buffer = buffer.slice(lens[i] * 4);
                }

                out[varnames[i]].push(val)
            }
        }
    }
    for (i=0; i<n_children; i++)
    {
        [buffer, childout, childname] = blobDecode(buffer);
        out[childname] = childout;
    }
    
    return [buffer, out, nodename];
}
