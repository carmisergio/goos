#include "proc/proc.h"

// Global objects
proc_cb_t cur_proc;

proc_cb_t *proc_cur()
{
    return &cur_proc;
}