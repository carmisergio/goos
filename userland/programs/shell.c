#include "stdio.h"
#include "goos.h"
#include <stdbool.h>
#include "string.h"

// Maximums
#define MAX_ARGS_N 16
#define MAX_ARG 64
#define MAX_LINE 256

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

// Builtin commands
#define BUILTIN_CLEAR "clear"

void clear_console()
{
    puts("\033[2J\033[H");
}

void print_splash_message()
{
    printf("Welcome to %s\n", COLOR_HI_BLUE);
    puts("  __ _  ___   ___  ___  \n");
    puts(" / _` |/ _ \\ / _ \\/ __| \n");
    puts("| (_| | (_) | (_) \\__ \\\n");
    puts(" \\__, |\\___/ \\___/|___/ \n");
    puts("  __/ |                 \n");
    printf(" |___/                  %sv0.0.1 \n", COLOR_RESET);
    puts("\n");
}

void print_prompt()
{
    char cwd[PATH_MAX + 1];
    cwd[0] = 0;

    // Get current working directory
    get_cwd(cwd);

    // Print prompt
    printf("%s[goos %s]$%s ", COLOR_HI_BLUE, cwd, COLOR_RESET);
}

bool handle_builtins(const char *cmd)
{
    if (strcmp(cmd, BUILTIN_CLEAR) == 0)
    {
        clear_console();
        return true;
    }

    return false;
}

void process_input(const char *input)
{
    // Handle empty input
    if (strlen(input) == 0)
        return;

    // Handle builtin commands
    if (handle_builtins(input))
        return;

    // Process input
    // For now, just execute program
    int32_t res, status;
    if ((res = exec(input, &status)) < 0)
    {
        printf("Error: %s\n", error_get_message(res));
        return;
    }

    // Print process exit value
    printf("\nProcess exited with status %d\n", status);
}

int main()
{
    clear_console();

    print_splash_message();

    while (true)
    {

        // Print prompt
        print_prompt();

        // Read user input
        char linebuf[MAX_LINE + 1];
        getsn(linebuf, MAX_LINE);

        // Process user input
        process_input(linebuf);
    }
}