#include "fs/path.h";

/*
 * Consume single known character
 */
bool parse_ctag(char **input, char tag)
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
bool parse_uint32(uint32_t *res, char **input)
{
    bool has_num = false;
    char *cur = *input;
    *res = 0;
    while (*cur)
    {
        // Check if current if valid digit
        if (*cur < '0' || *cur > '9')
            break;

        // Read number
        *res *= 10;
        *res += *cur - '0';

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

/*
 * Parse mountpoint from file path
 * Consumes the ':' character
 */
bool parse_path_mountpoint(mount_point_t *mp, char **input)
{
    char *cur = *input;

    // Parse mountpoint number
    if (!parse_uint32(mp, &cur))
        return false;

    // Parse ':' character
    if (!parse_ctag(&cur, ':'))
        return false;

    *input = cur;

    return true;
}

/*
 * Parse filename from path
 * NOTE: name must be a pointer to a buffer of at least MAX_FILENAME + 1 bytes
 * Ignores character after MAX_FILENAME
 */
bool parse_path_filename(char *name, char **input)
{
    char *cur = *input;

    // Consume leading '/' (possibly multiple, possibly 0)
    while (parse_ctag(&cur, '/'))
        ;

    uint32_t n = 0;
    while (*cur)
    {
        // Check if current is not slash
        if (*cur == '/')
            break;

        // Add character to path
        if (n != FILENAME_MAX)
        {
            name[n] = *cur;
            n++;
        }
        cur++;
    }

    if (n != 0)
    {
        // Success
        name[n] = '\0'; // Null terminate name
        *input = cur;
        return true;
    }
    else
    {
        // Failure
        return false;
    }
}