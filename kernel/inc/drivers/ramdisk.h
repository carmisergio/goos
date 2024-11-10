#pragma once

// Ramdisk block device

#include <stdint.h>

// Initialize ramdiske
void ramdisk_create(uint32_t id, uint32_t nblocks);
