#include "stdio.h"
#include "string.h"
#include <stdint.h>

#define MSG_N 32
#define REPEAT_MAX 64

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

static void print_cow(const char *msg);
static void get_msg(char *msg);
static void repeat_char(char *spacer, char c, uint32_t n);

int main()
{
    char msg_buf[MSG_N + 1];

    // Get message
    get_msg(msg_buf);

    // Print cow
    print_cow(msg_buf);
}

static void get_msg(char *msg)
{
    // Print prompt
    putss("What does the cow say? ");

    // Get input
    getsn(msg, MSG_N);
}

static void print_cow(const char *msg)
{
    char rpt_buf[REPEAT_MAX + 1];
    uint32_t msg_len = strlen(msg);

    // Top line
    putss("  ");
    repeat_char(rpt_buf, '_', msg_len);
    puts(rpt_buf);

    // Message line
    printf("< %s", COLOR_HI_RED);
    putss(msg);
    printf("%s >", COLOR_RESET);

    // Bottom line
    putss("\n  ");
    repeat_char(rpt_buf, '-', msg_len);
    puts(rpt_buf);

    repeat_char(rpt_buf, ' ', msg_len / 2);
    printf("%s  \\   ^__^\n", rpt_buf);
    printf("%s   \\  (oo)\\_______\n", rpt_buf);
    printf("%s      (__)\\       )\\/\\\n", rpt_buf);
    printf("%s          ||----w |\n", rpt_buf);
    printf("%s          ||     ||\n", rpt_buf);
}

static void repeat_char(char *spacer, char c, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        spacer[i] = c;
    }
    spacer[n] = 0;
}
