#ifndef _DRIVERS_SERIAL_H
#define _DRIVERS_SERIAL_H 1

#include <stdint.h>

// Represents a  serial port
enum com_port
{
    COM1 = 0x3F8,
    COM2 = 0x2F8,
    COM3 = 0x3E8,
    COM4 = 0x2E8,
    COM5 = 0x5F8,
    COM6 = 0x4F8,
    COM7 = 0x5E8,
    COM8 = 0x4E8,

};

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Initialize serial port output
     *
     * Params:
     *     com_port port: port to output to
     * Returns: void
     */
    int serial_init(enum com_port port);

    /*
     * Output character to serial port
     *
     * Params:
     *     com_port port: port to output to
     *     char c: character to output to serial
     * Returns: void
     */
    void serial_putchar(enum com_port port, char c);

    /*
     * Print null terminated string to serial port
     *
     * Params:
     *     com_port port: port to output to
     *     char *str: string to print
     * Returns: void
     */
    void serial_prtstr(enum com_port port, char *str);

#ifdef __cplusplus
}
#endif

#endif