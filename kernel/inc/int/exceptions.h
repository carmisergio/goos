#ifndef _INT_EXCEPTIONS_H
#define _INT_EXCEPTIONS_H

#include "int/interrupts.h"

/*
 * Handle exception
 */
void handle_exception(interrupt_context_t *ctx);

#endif