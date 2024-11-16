#pragma once

#include "fs/vfs.h"

/*
 * Load an ELF executable into the current UVAS
 * #### Parameters:
 *   - file: file handle of the executable
 *   - entry: pointer to entry point pointer, will be set to
 *            the entry point of the program
 *  #### Returns:
 *    0 on success, error number on failure
 *  NOTE: doesn't clean up any allocated memory in the UVAS on failure
 */
int32_t elf_load(vfs_file_handle_t file, void **entry);