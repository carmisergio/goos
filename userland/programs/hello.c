#include "stdio.h"

int main()
{
    int a = 0;
    while (1)
    {
        char name[32];
        printf("What is your name? ");
        getsn(name, 32);
        printf("Hello, %s! %d\n", name, a);
        a++;
    }
}