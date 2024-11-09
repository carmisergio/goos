#include "drivers/cmos.h"

#include "sys/io.h"

#define CMOS_PORT_CMD 0x70
#define CMOS_PORT_DATA 0x71

uint8_t cmos_read_reg(cmos_reg_t reg)
{
    // Keep NMI disabled
    outb(CMOS_PORT_CMD, (1 << 7) | reg);
    io_delay();

    // Read value
    return inb(CMOS_PORT_DATA);
}