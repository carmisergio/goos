#include "fs/path.h"

#include "string.h"
#include "mini-printf.h"

#include "log.h"

// Internal functions
bool parse_ctag(const char **input, char tag);
bool parse_uint32(uint32_t *res, const char **input);
bool put_mountpoint(char *path, uint32_t *n, mount_point_t mp);
bool put_filename(char *path, uint32_t *n, const char *name);
bool pop_filename(char *path, uint32_t *n);

/*
 * Parse mountpoint from file path
 * Consumes the ':' character
 */
bool path_parse_mountpoint(mount_point_t *mp, const char **input)
{
    const char *cur = *input;

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
bool path_parse_filename(char *name, const char **input)
{
    const char *cur = *input;

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

bool path_canonicalize(char *dst, const char *src)
{
    const char *src_cur = src;

    // Get mountpoint
    mount_point_t mp;
    if (!path_parse_mountpoint(&mp, &src_cur))
        return false;

    // Counter of characters put in the output buffer
    uint32_t n = 0;

    // Write mountpoint to canonicalized path
    if (!put_mountpoint(dst, &n, mp))
        return false;

    // Append file paths to canonicalized path
    char name[FILENAME_MAX + 1];
    while (path_parse_filename(name, &src_cur))
    {
        // Canonicalized path cannot contain relative filenames
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            return false;

        if (!put_filename(dst, &n, name))
            return false;
    }

    // NULL-terminate path
    dst[n] = 0;

    return true;
}

bool path_resolve_relative(char *dst, const char *cwd, const char *relpath)
{
    const char *relpath_cur = relpath;

    // Result counter
    uint32_t n = 0;

    // Get mountpoint from relative path
    mount_point_t mp;
    if (path_parse_mountpoint(&mp, &relpath_cur))
    {
        // If it does have a mountpoint, use that as the base for the result
        if (!put_mountpoint(dst, &n, mp))
            return false;
    }
    else
    {
        // Otherwise, use the CWD as the base
        // Copy CWD into result buffer
        uint32_t cwd_len = strlen(cwd);
        memcpy(dst, cwd, cwd_len);

        // Put result counter at end of CWD
        n = cwd_len;
    }

    // Parse relative path filename by filename
    char name[FILENAME_MAX + 1];
    while (path_parse_filename(name, &relpath_cur))
    {

        // Handle special cases
        if (strcmp(name, ".") == 0)
            continue;
        if (strcmp(name, "..") == 0)
        {
            // Go back 1 directory
            if (!pop_filename(dst, &n))
                return false;

            continue;
        }

        // Go forward
        if (!put_filename(dst, &n, name))
            return false;
    }

    // Null-terminate result
    dst[n] = 0;

    return true;
}

/* Internal functions */

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
    bool has_num = false;
    const char *cur = *input;
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

// Append mountpoint to a path
bool put_mountpoint(char *path, uint32_t *n, mount_point_t mp)
{
    // Get length of mountpoint string
    uint32_t len = snprintf(NULL, PATH_MAX, "%u:", mp);

    // Check if it fits
    if (len > PATH_MAX - *n)
        return false;

    // Do put
    snprintf(path + *n, len + 1, "%u:", mp);
    *n += len;

    return true;
}

// Append filename to a path
bool put_filename(char *path, uint32_t *n, const char *name)
{
    // Get length of filename (+1 for /)
    uint32_t name_len = strlen(name);
    uint32_t len = name_len + 1;

    // Check if it fits
    if (len > PATH_MAX - *n)
        return false;

    // Do put
    path[*n] = '/';
    memcpy(path + *n + 1, name, name_len);
    *n += len;

    return true;
}

// Remove filename from end of path
// Can only be applied to a canonicalized path
bool pop_filename(char *path, uint32_t *n)
{
    for (int32_t i = *n - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        {
            *n = i;
            return true;
        }
    }

    return false;
}
