#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/blob.h"

#define NELEM 100


int
main(int argc, char **argv)
{
    int j = 0;
    unsigned int abs_j = 0;
    int jval = 0;
    float jval_squared = 0.0f;
    float jval_cubed = 0.0f;

    if (argc != 2)
    {
        printf("Usage: %s <blob_server_ip>\n", argv[0]);
    }

    BLOB_INIT(argv[1], 8000);
    while (1)
    {
        abs_j = abs(jval);
        BLOB_START("main");
        BLOB_INT_A("jval", &jval, 1);
        jval_squared = 1.0 * (jval) * (jval) / (NELEM / 2);
        jval_cubed = 1.0 * (jval) * (jval) * (jval) / (NELEM * NELEM / 4);
        BLOB_FLOAT_A("jval_squared", &jval_squared, 1);
        BLOB_FLOAT_A("jval_cubed", &jval_cubed, 1);
        BLOB_UNSIGNED_INT_A("abs_j", &abs_j, 1);

        jval =  ((j + 1) % NELEM) - NELEM / 2;
        j++;
        usleep(20000); // 20ms
        BLOB_FLUSH();
    }
    
    BLOB_TERMINATE();
    return 0;
}
