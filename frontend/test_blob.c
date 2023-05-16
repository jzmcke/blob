#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/blob.h"

#define NUM_IN_FIRST_ARRAY  (4)
#define NUM_IN_SECOND_ARRAY (7)
#define NUM_IN_THIRD_ARRAY (170)
#define NELEM 100

void
func_top(void)
{
    int ret;
    int a_array_first[NUM_IN_FIRST_ARRAY];
    int a_array_second[NUM_IN_SECOND_ARRAY];
    
    static int start = 0;
    BLOB_START("top");
    for (int i=0; i<NUM_IN_FIRST_ARRAY; i++)
    {
        a_array_first[i] = start + i;
    }
    for (int i=0; i<NUM_IN_SECOND_ARRAY; i++)
    {
        a_array_second[i] = start + i;
    }

    ret = BLOB_INT_A("first_array", a_array_first, NUM_IN_FIRST_ARRAY);
    assert(ret==0);
    ret = BLOB_INT_A("second_array", a_array_second, NUM_IN_SECOND_ARRAY);
    assert(ret==0);
    BLOB_FLUSH();
    start = start + 100;
}

void
func_mid(void)
{
    int ret;
    int integer_val = -1;
    unsigned int unsigned_int_val = 4;
    float a_array_first[NUM_IN_FIRST_ARRAY];

    BLOB_START("top_second");
    for (int i=0; i<NUM_IN_FIRST_ARRAY; i++)
    {
        a_array_first[i] = i+0.517;
    }
    BLOB_START("mid");
    ret = BLOB_INT_A("integer_val", &integer_val, 1);
    assert(ret ==  0);
    ret = BLOB_UNSIGNED_INT_A("unsigned_integer_val", &unsigned_int_val, 1);
    assert(ret ==  0);
    ret = BLOB_FLOAT_A("float_array", a_array_first, NUM_IN_FIRST_ARRAY);
    assert(ret ==  0);
    BLOB_FLUSH();
    BLOB_FLUSH();
}

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
