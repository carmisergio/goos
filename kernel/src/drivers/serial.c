#include <sys/io.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "drivers/serial.h"

static bool _transmit_ready(enum com_port port);

// TODO: Add serial port settings to serial_init()

/* Public functions */
int serial_init(enum com_port port)
{
    outb(port + 1, 0x00); // Disable all interrupts
    outb(port + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00); //                  (hi byte)
    outb(port + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B); // IRQs enabled, RTS/DSR set
    outb(port + 4, 0x1E); // Set in loopback mode, test the serial chip
    outb(port + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (inb(port + 0) != 0xAE)
    {
        return -1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(port + 4, 0x0F);
    return 0;
}

void serial_putchar(enum com_port port, char c)
{
    while (!_transmit_ready(port))
        ;

    outb(port, c);
}

void serial_prtstr(enum com_port port, char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
    {
        // Expand \n to \r\n
        if (str[i] == '\n')
            serial_putchar(port, '\r');

        serial_putchar(port, str[i]);
    }
}

/* Internal functions */

/*
 * Checks if the serial port is ready to trasmit
 *
 * Params:
 *     uint8_t c: character to output to serial
 *     com_port port: port to check
 * Returns: void
 */
static bool _transmit_ready(enum com_port port)
{
    return (inb(port + 5) & 0x20) != 0;
}
