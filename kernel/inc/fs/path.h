#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fs/vfs.h"

/*
 * Consume single known character
 */
bool parse_ctag(const char **input, char tag);

/*
 * Parse unsigned 32 bit number
 */
bool parse_uint32(uint32_t *res, const char **input);

/*
 * Parse mountpoint from file path
 * Consumes the ':' character
 */
bool parse_path_mountpoint(mount_point_t *mp, const char **input);

/*
 * Parse filename from path
 * NOTE: name must be a pointer to a buffer of at least MAX_FILENAME + 1 bytes
 * Ignores character after MAX_FILENAME
 */
bool parse_path_filename(char *name, const char **input);