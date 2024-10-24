#ifndef _SYSREQ_H
#define _SYSREQ_H 1

// This module handles direct input (keyboard shortcuts)
// which triggers actions directly on the kernel

/*
 * Initialize system request handler
 */
void sysreq_init();

/*
 * Finalize system request handler
 */
void sysreq_finalize();

#endif