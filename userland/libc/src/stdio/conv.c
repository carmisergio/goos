#include "stdio.h"

#include <stdbool.h>
#include <stdint.h>

//
// String conversion functions
//

// Internal function prototypes
bool parse_ctag(const char **input, char tag);
bool parse_uint32(uint32_t *res, const char **input);

int32_t strtoi(const char *str)
{
    // Check if there is a - sign
    bool neg = parse_ctag(&str, '-');

    // Parse digits
    uint32_t n;
    if (!parse_uint32(&n, str))
        return 0;

    if (neg)
        n = -n;

    return n;
}

// Internal functions
/*
 * Consume single known character
 */
bool parse_ctag(const char **input, char tag)
{
    if (**input == tag)
    {
        (*input)++;
        return true;
    }

    return false;
}

/*
 * Parse unsigned 32 bit number
 */
bool parse_uint32(uint32_t *res, const char **input)
{
    const char *cur = *input;
    *res = 0;
    uint8_t digits = 0; // Number of digits read
    while (*cur && digits < 10)
    {
        // Check if current if valid digit
        if (*cur < '0' || *cur > '9')
            break;

        // Read number
        *res *= 10;
        *res += *cur - '0';

        cur++;
        digits++;
    }

    if (digits == 0)
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
