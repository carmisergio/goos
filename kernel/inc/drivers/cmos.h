#pragma once

#include <stdint.h>

typedef enum
{
    CMOS_REG_FLPTYPE = 0x10,
} cmos_reg_t;

// Floppy types
#define CMOS_FLPTYPE_NONE 0x0
#define CMOS_FLPTYPE_525_360K 0x1
#define CMOS_FLPTYPE_525_12M 0x2
#define CMOS_FLPTYPE_35_720K 0x3
#define CMOS_FLPTYPE_35_144M 0x4
#define CMOS_FLPTYPE_35_288M 0x5

// Read CMOS register
uint8_t cmos_read_reg(cmos_reg_t reg);