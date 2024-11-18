#include "stdlib.h"

#include "goos.h"

void exit(int status)
{
    _g_exit(status);
}