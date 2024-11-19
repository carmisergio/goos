#include "parse.h"

// Parse any character
bool parse_anychar(const char **input, char *c)
{
    // No charactesr to read
    if (!**input)
        return false;

    *c = **input;
    (*input)++;
    return true;
}

// Parse positive integer
bool parse_uint32(const char **input, uint32_t *n)
{
    bool has_num = false;
    const char *cur = *input;
    *n = 0;
    while (*cur)
    {
        // Check if current if valid digit
        if (*cur < '0' || *cur > '9')
            break;

        // Read number
        *n *= 10;
        *n += *cur - '0';

        has_num = true;

        cur++;
    }

    if (has_num)
    {
        // Success
        *input = cur;
        return true;
    }
    else
    {
        // Failure
        return false;
    }
}

// Consume single known character
bool parse_ctag(const char **input, char tag)
{
    if (**input == tag)
    {
        (*input)++;
        return true;
    }

    return false;
}