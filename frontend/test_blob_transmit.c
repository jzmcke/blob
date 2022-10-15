#include "blob/include/blob.h"
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <unistd.h> 

int
main(int argc, char **argv)
{
    static struct termios oldt, newt;
    char c;
    /*tcgetattr gets the parameters of the current terminal
    STDIN_FILENO will tell tcgetattr that it should write the settings
    of stdin to oldt*/
    tcgetattr( STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
    newt = oldt;

    /*ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL*/
    newt.c_lflag &= ~(ICANON);          

    /*Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    BLOB_INIT("192.168.50.115", 8000);
    
    while ((c = getchar()))
    {
        int forward = 0;
        int backward = 0;
        int left = 0;
        int right = 0;
        int stop = 0;
        

        if (c == 'w')
        {
            forward = 1;
        }
        else if (c == 's')
        {
            backward = 1;
        }
        else if (c == 'x')
        {
            stop = 1;
        }
        else if (c == 'a')
        {
            left = 1;
        }
        else if (c == 'd')
        {
            right = 1;
        }
        else
        {
            continue;
        }
        BLOB_START("main");
        BLOB_INT_A("forward", &forward, 1);
        BLOB_INT_A("backward", &backward, 1);
        BLOB_INT_A("left", &left, 1);
        BLOB_INT_A("right", &right, 1);
        BLOB_INT_A("stop", &stop, 1);
        BLOB_FLUSH();
        usleep(100000); // 100000ms
    }
    
    BLOB_TERMINATE();
    return 0;
}
