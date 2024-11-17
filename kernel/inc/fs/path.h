#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fs/vfs.h"

/*
 * Parse mountpoint from file path
 * Consumes the ':' character
 */
bool path_parse_mountpoint(mount_point_t *mp, const char **input);

/*
 * Parse filename from path
 * NOTE: name must be a pointer to a buffer of at least MAX_FILENAME + 1 bytes
 * Ignores character after MAX_FILENAME
 */
bool path_parse_filename(char *name, const char **input);

/*
 * Convert a path to canonic form
 * NOTE: dst must be a pointer to a buffer of at least MAX_PATH + 1 bytes
 * Can fail if the path doesn't contain a mountpoint
 */
bool path_canonicalize(char *dst, const char *src);

/*
 * Resolve relative path
 * NOTE: dst must be a pointer to a buffer of at least MAX_PATH + 1 bytes
 * NOTE: cwd must be a canonical path
 * Produces canonicalized paths
 */
bool path_resolve_relative(char *dst, const char *cwd, const char *relpath);