#include <stdbool.h>
#include "stdio.h"

char *msg = "Hello from child!\n";

// Extremely minimal C program
int main()
{
    puts(msg);
    while (true)
    {
        char c = getchar();
        printf("Character: %c\n", c);
    }

    return 0;
}