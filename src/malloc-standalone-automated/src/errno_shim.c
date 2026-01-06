// src/errno_shim.c
#include <errno.h>

void __set_errno (int e)
{
    errno = e;
}
