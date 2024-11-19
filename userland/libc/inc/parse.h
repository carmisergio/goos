#pragma once

#include <stdbool.h>
#include <stdint.h>

// Parse any character
bool parse_anychar(const char **input, char *c);

// Parse positive integer
bool parse_uint32(const char **input, uint32_t *n);

// Consume single known character
bool parse_ctag(const char **input, char tag);