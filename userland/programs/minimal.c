#include <string.h>
#include <stdbool.h>
#include "goos.h"

char *msg = "Hello from minimal.c! \e[31mRED \e[32mGREEN \e[34mBLUE \e[0m\n";

// Extremely minimal C program
int main()
{
    console_write(msg, strlen(msg));

    // while (true)
    // {
    // for (int i = 0; i < 10; i++)
    // {
    int32_t retval;
    if (exec("0:/BIN/CHILD", &retval) < 0)
    {
        console_write("fail!\n", 6);
    };

    if (retval < 0)
        console_write("proc fail!\n", 11);
    // }
    // }
    //

    // The problem now is that the data segment selectors are not set correctly
    // while in the system call context
    // This might not be the full issue

    console_write(msg, strlen(msg));

    // if (exit(123) < 0)
    // {
    //     console_write("fail!", 5);
    // }

    return 0;
}