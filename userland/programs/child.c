#include <string.h>
#include <stdbool.h>
#include "goos.h"

char *msg = "Hello from child!\n";

// Extremely minimal C program
int main()
{
    // console_write((char *)0x20, strlen(msg));
    // *(int *)0x20 = 10;
    while (true)
    {
        console_write(msg, strlen(msg));

        delay_ms(1000);
    }

    return 0;
}