#include <string.h>
#include <stdbool.h>
#include "goos.h"

char *msg = "Hello from child!\n";
char *msg_fail = "cwd fail!\n";

// Extremely minimal C program
int main()
{
    // console_write((char *)0x20, strlen(msg));
    // *(int *)0xc010089e = 10;
    while (true)
    {
        console_write(msg, strlen(msg));

        char cwd[PATH_MAX + 1];
        if (get_cwd(cwd) < 0)
            console_write(msg_fail, strlen(msg_fail));
        else
            console_write(cwd, strlen(cwd));
        console_write("\n", 1);

        delay_ms(1000);
    }

    return 0;
}