#include "stdio.h"
#include "goos.h"
#include <stdbool.h>
#include "string.h"
#include "parse.h"

#define CONSOLE_HEIGHT 25
#define CONSOLE_WIDTH 80

// Maximums
#define ARGS_N 16
#define MAX_ARG 64
#define MAX_LINE 256

#define LS_BUF_N 24 // Matched to number of lines

// Colors
#define COLOR_RESET "\033[0m"
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_PURPLE "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_HI_BLACK "\033[90m"
#define COLOR_HI_RED "\033[91m"
#define COLOR_HI_GREEN "\033[92m"
#define COLOR_HI_YELLOW "\033[93m"
#define COLOR_HI_BLUE "\033[94m"
#define COLOR_HI_PURPLE "\033[95m"
#define COLOR_HI_CYAN "\033[96m"
#define COLOR_HI_WHITE "\033[97m"

typedef char argv_t[MAX_ARG + 1];

// Builtin command table entry
typedef struct
{
    char *cmd;                        // Command name
    void (*func)(uint32_t, argv_t *); //

} builtin_cmd_t;

// Internal functions
static void main_loop();
static void print_splash_message();
static inline void clear_console();
static void print_prompt();
size_t parse_input(argv_t *argv, const char *input);
bool parse_arg(const char **input, char *arg);
static bool handle_builtins(uint32_t argc, argv_t *argv);
static void execute_program(uint32_t argc, argv_t *argv);
static void builtin_clear(uint32_t argc, argv_t *argv);
static void builtin_cd(uint32_t argc, argv_t *argv);
static void builtin_exit(uint32_t argc, argv_t *argv);
static void builtin_unmount(uint32_t argc, argv_t *argv);
static void builtin_mount(uint32_t argc, argv_t *argv);
static void builtin_ls(uint32_t argc, argv_t *argv);
static void builtin_ls_display_dirent(dirent_t *dirent);
static bool enter_or_quit();

// Builtin command table
static builtin_cmd_t builtin_cmds[] = {
    {
        .cmd = "clear",
        .func = builtin_clear,
    },
    {
        .cmd = "cd",
        .func = builtin_cd,
    },
    {
        .cmd = "unmount",
        .func = builtin_unmount,
    },
    {
        .cmd = "mount",
        .func = builtin_mount,
    },
    {
        .cmd = "ls",
        .func = builtin_ls,
    },
    {
        .cmd = "exit",
        .func = builtin_exit,
    }};

// void process_input(const char *input)
// {
//     // Handle empty input
//     if (strlen(input) == 0)
//         return;

//     // Handle builtin commands
//     if (handle_builtins(input))
//         return;

//     // Process input
//     // For now, just execute program
//     int32_t res, status;
//     if ((res = _g_exec(input, &status)) < 0)
//     {
//         printf("Error: %s\n", error_get_message(res));
//         return;
//     }

//     // Print process exit value
//     putss(COLOR_RESET); // Reset color
//     printf("\nProcess exited with status %d\n", status);
// }

int main()
{
    clear_console();
    print_splash_message();
    main_loop();
}

static void main_loop()
{
    while (true)
    {

        // Print prompt
        print_prompt();

        // Read user input
        char linebuf[MAX_LINE + 1];
        getsn(linebuf, MAX_LINE);

        // Parse user input into arguments
        char argv[ARGS_N][MAX_ARG + 1];
        uint32_t argc = parse_input(argv, linebuf);

        // Ignore empty input
        if (!argc)
            continue;

        // Handle executing builtin commmands
        // They take priority over executables
        if (handle_builtins(argc, argv))
            continue;

        // Execute program
        execute_program(argc, argv);
    }
}

// Print initial splash message to screen
static void print_splash_message()
{
    printf("Welcome to %s\n", COLOR_HI_BLUE);
    putss("  __ _  ___   ___  ___  \n");
    putss(" / _` |/ _ \\ / _ \\/ __| \n");
    putss("| (_| | (_) | (_) \\__ \\\n");
    putss(" \\__, |\\___/ \\___/|___/ \n");
    putss("  __/ |                 \n");
    printf(" |___/                  %sv0.0.1 \n", COLOR_RESET);
    putss("\n");
}

// Clear system console
static inline void clear_console()
{
    putss("\033[2J\033[H");
}

// Print shell prompt
static void print_prompt()
{
    char cwd[PATH_MAX + 1];

    // Get current working directory
    _g_get_cwd(cwd);

    // Print prompt
    printf("%s[goos %s]$%s ", COLOR_HI_BLUE, cwd, COLOR_RESET);
}

// Process user input
size_t parse_input(argv_t *argv, const char *input)
{

    size_t n_args = 0;

    // Parse all arguments
    while (n_args < ARGS_N)
    {
        if (!parse_arg(&input, argv[n_args]))
            return n_args;

        n_args++;
    }

    return n_args;
}

// Parse one input argument from the user input
bool parse_arg(const char **input, char *arg)
{
    size_t n = 0; // Number of characrters consumed

    const char *cur = *input;

    // Consume any leading spaces
    while (parse_ctag(&cur, ' '))
        ;

    // Consume to end of line or space character
    while (*cur != 0 && *cur != ' ')
    {
        if (n < MAX_ARG)
        {
            // Add character to a*rgument
            arg[n] = *cur;
            n++;
        }

        // Next character
        cur++;
    }

    // No characters consumed
    if (n == 0)
        return false;

    // NULL-terminate arg
    arg[n] = 0;

    // Advance input
    *input = cur;

    return true;
}

// Handle executing builtin commands
static bool handle_builtins(uint32_t argc, argv_t *argv)
{
    size_t n_builtins = sizeof(builtin_cmds) / sizeof(builtin_cmd_t);

    // Get command
    char *cmd = argv[0];

    // Iterate over all known builtins
    for (size_t i = 0; i < n_builtins; i++)
    {
        // Check if command matches
        if (strcmp(cmd, builtin_cmds[i].cmd) == 0)
        {
            // Execute command
            builtin_cmds[i].func(argc, argv);
            return true;
        }
    }

    return false;
}

// Handle executing a program
static void execute_program(uint32_t argc, argv_t *argv)
{
    size_t n_builtins = sizeof(builtin_cmds) / sizeof(builtin_cmd_t);

    // Get command
    char *cmd = argv[0];

    int32_t res, status;
    if ((res = _g_exec(cmd, &status)) < 0)
    {
        if (res == E_NOENT || res == E_NOTELF || res == E_WRONGTYPE)
            // Command not found
            printf("%scommand not found%s: %s\n", COLOR_HI_RED, COLOR_RESET, cmd);
        else
            // exec() call error
            printf("exec(): %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(res));
        return;
    }

    // Print process exit value
    putss(COLOR_RESET); // Reset color
    printf("\nProcess exited with status %d\n", status);

    return;
}

// "clear" builtin command
static void builtin_clear(uint32_t argc, argv_t *argv)
{
    clear_console();
}

// "cd" builtin command
static void builtin_cd(uint32_t argc, argv_t *argv)
{

    // Check if number of arguments is correct
    if (argc != 2)
    {
        puts("Usage: cd <path>");
        return;
    }

    // Read arguments
    char *path = argv[1];

    // Execute change dir
    int32_t res;
    if ((res = _g_change_cwd(path)) < 0)
    {
        if (res == E_NOENT)
            // Command not found
            printf("cd: no such directory: %s\n", path);
        else if (res == E_WRONGTYPE)
            printf("cd: not a directory: %s\n", path);
        else
            // cd() call error
            printf("cd: %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(res));
        return;
    }
}

// "exit" builtin command
static void builtin_exit(uint32_t argc, argv_t *argv)
{

    // Execute exit
    int32_t res;
    if ((res = _g_exit(0)) < 0)
    {
        printf("exit(): %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(res));
        return;
    }
}

// "unmount" builtin command
static void builtin_unmount(uint32_t argc, argv_t *argv)
{
    // Check if number of arguments is correct
    if (argc != 2)
    {
        puts("Usage: unmount <mountpoint>");
        return;
    }

    // Parse mountpoint
    uint32_t mp;
    char *mp_arg = argv[1];
    if (!parse_uint32((const char **)&mp_arg, &mp))
    {
        printf("unmount: invalid mountpoint: %s\n", mp_arg);
        return;
    }

    // Execute unmount
    int32_t res;
    if ((res = _g_unmount(mp)) < 0)
    {
        // TODO: more specific error reporting
        printf("unmount: %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(res));
        return;
    }
}

// "mount" builtin command
static void builtin_mount(uint32_t argc, argv_t *argv)
{
    // Check if number of arguments is correct
    if (argc != 4)
    {
        puts("Usage: mount <mountpoint> <dev> <fs type>");
        return;
    }

    // Parse mountpoint
    uint32_t mp;
    const char *mp_arg = argv[1];
    if (!parse_uint32(&mp_arg, &mp))
    {
        printf("mount: invalid mountpoint: %s\n", mp_arg);
        return;
    }

    const char *dev = argv[2];
    const char *fs_type = argv[3];

    // Execute unmount
    int32_t res;
    if ((res = _g_mount(mp, dev, fs_type)) < 0)
    {
        if (res == E_NOENT)
            printf("mount: %serror%s: device not existent or already mounted\n", COLOR_HI_RED, COLOR_RESET);
        else if (res == E_NOMP)
            return;
        else
            printf("mount: %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(res));
        return;
    }
}

// Builtin "ls" command
static void builtin_ls(uint32_t argc, argv_t *argv)
{
    fd_t fd;
    int32_t res;

    if (argc != 1 && argc != 2)
        puts("Usage: ls [<path>]");

    // Select path if provided
    char *path = ".";
    if (argc == 2)
        path = argv[1];

    // Open directory
    if ((fd = _g_open(path, FOPT_DIR)) < 0)
    {
        if (fd == E_NOENT)
            printf("ls: no such directory: %s\n", path);
        else if (fd == E_WRONGTYPE)
            printf("ls: not a directory: %s\n", path);
        else
            printf("ls: %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(fd));
        return;
    }

    dirent_t dir_buf[LS_BUF_N];
    uint32_t offset = 0;

    uint32_t total_bytes = 0;
    uint32_t lines_printed = 0;

    do
    {
        // Read directory
        res = _g_readdir(fd, dir_buf, offset, LS_BUF_N);

        // Check result
        if (res < 0)
        {
            printf("ls: %serror%s: %s\n", COLOR_HI_RED, COLOR_RESET, error_get_message(res));
            goto end;
        }

        // Display result
        for (size_t i = 0; i < res; i++)
        {
            builtin_ls_display_dirent(&dir_buf[i]);
            total_bytes += dir_buf[i].size;

            // Stop outout when scrolling out of view
            if (lines_printed >= CONSOLE_HEIGHT - 1)
            {
                lines_printed = 0;
                if (!enter_or_quit())
                {
                    goto end;
                }
            }

            lines_printed++;
        }

        offset += res;

    } while (res >= LS_BUF_N);

    // Print total bytes
    printf("Total bytes: %u\n", total_bytes);

end:
    // Close directory
    _g_close(fd);
}

static void builtin_ls_display_dirent(dirent_t *dirent)
{

    char *color = COLOR_RESET;
    char type = 'f';
    if (dirent->type == FTYPE_DIR)
    {
        color = COLOR_HI_BLUE;
        type = 'd';
    }

    printf("%c %6u %s%s%s\n", type, dirent->size, color, dirent->name, COLOR_RESET);
}

// Ask user to press enter to continue, Q to quit
static bool enter_or_quit()
{
    putss("Press [ENTER] to continue, [q] to quit");

    // Wait for keypress
    while (1)
    {
        char c = getchar();

        if (c == '\n')
        {
            putss("\n");
            return true;
        }
        else if (c == 'q')
        {
            putss("\n");
            return false;
        }
    }
}