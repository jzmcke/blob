#include "blob_jbuf_frag.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define N_BLOB_TREES 100
#define TOTAL_FRAG_CNT 3
#define JBUF_LEN 8

blob_jbuf_cfg jbuf_cfg;

void*
create_data_blob(int seq_num, int frag_idx, int total_frag_cnt)
{
    // Length 4 to contain seq_num, frag_idx, total_frag_cnt, and payload
    void *p_blob = malloc(4 * sizeof(int));

    // Set seq_num, frag_idx, and total_frag_cnt
    int *p_blob_int = (int *)p_blob;
    p_blob_int[0] = seq_num;
    p_blob_int[1] = frag_idx;
    p_blob_int[2] = total_frag_cnt;
    p_blob_int[3] = seq_num * 1000 + frag_idx * 1;
    return p_blob;
}

int
main(int argc, char **argv)
{
    void *aap_blob_trees[N_BLOB_TREES][TOTAL_FRAG_CNT];
    void *p_out = NULL;
    int seq_num = 0;
    int frag_idx = 0;
    int ret = 0;
    size_t out_n = 0;
    blob_jbuf *p_jbuf = NULL;

    for (seq_num=0; seq_num<N_BLOB_TREES; seq_num++)
    {
        for (frag_idx=0; frag_idx<TOTAL_FRAG_CNT; frag_idx++)
        {
            aap_blob_trees[seq_num][frag_idx] = create_data_blob(seq_num, frag_idx, TOTAL_FRAG_CNT);
        }
    }

    jbuf_cfg.jbuf_len = JBUF_LEN;
    ret = blob_jbuf_init(&p_jbuf, &jbuf_cfg);
    assert(ret == BLOB_JBUF_OK);

    for (seq_num=0; seq_num<N_BLOB_TREES; seq_num++)
    {
        // push TOTAL_FRAG_CNTs for each pull event
        for (frag_idx=0; frag_idx<TOTAL_FRAG_CNT; frag_idx++)
        {
            ret = blob_jbuf_push(p_jbuf, aap_blob_trees[seq_num][frag_idx], 4 * sizeof(int));
            assert(ret == BLOB_JBUF_OK);
        }

        // Pull after every push
        ret = blob_jbuf_pull(p_jbuf, &p_out, &out_n);
    }




    return 0;

}