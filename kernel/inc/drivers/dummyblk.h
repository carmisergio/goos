#pragma once

// Dummy block device
// Responds to read operations by filling the buffer with id value and
// block offset alternatively

#include <stdint.h>

// Initialize a dummy block device
void dummyblk_init(uint32_t id, uint32_t nblocks);
