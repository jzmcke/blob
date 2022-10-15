#include "blob/include/blob.h"
#include <unistd.h>

int
main(int argc, char **argv)
{
    int i = 0;
    BLOB_SOCKET_INIT("localhost", 8000);
    
    while (1)
    {
        BLOB_START("main");
        BLOB_INT_A("i", &i, 1);
        BLOB_FLUSH();
        i = (i + 1) % 128;
        usleep(1000);
    }
    
    BLOB_SOCKET_TERMINATE();
    return 0;
}
