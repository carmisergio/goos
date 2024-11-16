#include "error.h"
#include <stdint.h>

char *error_get_message(int32_t err)
{
    switch (err)
    {
    case E_NOIMPL:
        return "not implemented";
    case E_NOENT:
        return "no such entity";
    case E_BUSY:
        return "resource busy";
    case E_TOOMANY:
        return "too many";
    case E_NOMP:
        return "no such mountpoint";
    case E_NOFS:
        return "no such filesystem type";
    case E_WRONGTYPE:
        return "wrong type";
    case E_IOERR:
        return "I/O error";
    case E_NOMEM:
        return "out of memory";
    case E_INCON:
        return "filesystem inconsistency";
    case E_NOTELF:
        return "not an ELF file";
    case E_ELFFMT:
        return "ELF format error";
    case E_NOTPERM:
        return "not permitted";
    case E_INVREQ:
        return "invalid request";
    case E_UNKNOWN:
    default:

        return "unknown error";
    }
}