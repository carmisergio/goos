#include <string.h>
#include "goos.h"

char *msg = "Hello from child!\n";

// Extremely minimal C program
int main()
{
    // console_write((char *)0x20, strlen(msg));
    // *(int *)0x20 = 10;
    console_write(msg, strlen(msg));

    return 0;
}