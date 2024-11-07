#pragma once

// Ramdisk block device

#include <stdint.h>

// Initialize ramdisk
void ramdisk_create(uint32_t id, uint32_t nblocks);
