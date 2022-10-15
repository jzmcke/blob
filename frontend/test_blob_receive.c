#include "blob/include/blob.h"
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

#define NUM_IN_FIRST_ARRAY  (4)
#define NUM_IN_SECOND_ARRAY (7)
#define NUM_IN_THIRD_ARRAY (170)
#define NELEM 100

int
main(int argc, char **argv)
{
    const int *p_up;
    const int *p_down;
    int up = 0, down = 0;
    int n_up;
    int n_down;
    int i=0;
    BLOB_INIT("192.168.50.115", 8000);
    
    while (1)
    {
        BLOB_RECEIVE_START("main");
        BLOB_RECEIVE_INT_A("forward", &p_up, &n_up, 0);
        if (NULL != p_up)
        {
            up = *p_up;
        }
        else
        {
            up = 0;
        }
        BLOB_RECEIVE_INT_A("backward", &p_down, &n_down, 0);
        if (NULL != p_down)
        {
            down = *p_down;
        }
        else
        {
            down = 0;
        }
        BLOB_RECEIVE_FLUSH();
        printf("%d: Received up %d\n", i, up);
        printf("%d: Received down %d\n", i, down);
        i++;
        usleep(100000); // 100ms
    }
    
    BLOB_TERMINATE();
    return 0;
}
