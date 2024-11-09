#include "drivers/fdc.h"

#include <stdint.h>
#include "sync.h"
#include "sys/io.h"

#include "log.h"
#include "drivers/cmos.h"
#include "clock.h"
#include "cpu.h"

#define DEBUG

#define CMD_TIMEOUT 50
#define CMD_RETRIES 3
#define RW_TIMEOUT 500
#define RW_RETRIES 5

// IO Ports
typedef enum
{
    PORT_STA = 0x3F0,  // Status A (read only)
    PORT_STB = 0x3F1,  // Status B (read only)
    PORT_DOR = 0x3F2,  // Data Output Register
    PORT_TDR = 0x3F3,  // Tape Drive Register
    PORT_MSR = 0x3F4,  // Main Status Register (read only)
    PORT_DSR = 0x3F4,  // Datarate Select Register (write only)
    PORT_FIFO = 0x3F5, // Data FIFO
    PORT_DIR = 0x3F7,  // Digital Input Register (read only)
    PORT_CCR = 0x3F7,  // Configuration Control Register (write only)
} io_port_t;

// FDC commands
typedef enum
{
    CMD_READ_TRACK = 2,
    CMD_SPECIFY = 3,
    CMD_SENSE_DRIVE_STATUS = 4,
    CMD_WRITE_DATA = 5,
    CMD_READ_DATA = 6,
    CMD_RECALIBRATE = 7,
    CMD_SENSE_INTERRUPT = 8,
    CMD_WRITE_DELETED_DATA = 9,
    CMD_READ_ID = 10,
    CMD_READ_DELETED_DATA = 12,
    CMD_FORMAT_TRACK = 13,
    CMD_DUMPREG = 14,
    CMD_SEEK = 15,
    CMD_VERSION = 16,
    CMD_SCAN_EQUAL = 17,
    CMD_PERPENDICULAR_MODE = 18,
    CMD_CONFIGURE = 19,
    CMD_LOCK = 20,
    CMD_VERIFY = 22,
    CMD_SCAN_LOW_OR_EQUAL = 25,
    CMD_SCAN_HIGH_OR_EQUAL = 29
} fdc_cmd_t;

#define CMD_BIT_MT (1 << 7)
#define CMD_BIT_MF (1 << 6)
#define CMD_BIT_SK (1 << 5)

// MSR structure
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t acta : 1;
        uint8_t actb : 1;
        uint8_t actc : 1;
        uint8_t actd : 1;
        uint8_t busy : 1; // CD
        uint8_t ndma : 1;
        uint8_t dio : 1;
        uint8_t rqm : 1;
    };
} msr_t;

// DOR structure
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t dsel : 2;
        uint8_t reset : 1;
        uint8_t irq : 1;
        uint8_t mota : 1;
        uint8_t motb : 1;
        uint8_t motc : 1;
        uint8_t motd : 1;
    };
} dor_t;

// Drives
typedef enum
{
    DRIVE_A = 'A',
    DRIVE_B = 'B',
} drive_t;

// Block device state
typedef struct
{
    drive_t drive;
    slock_t dev_lck;

    bool motor_state;

} fdc_drv_state_t;

// Intenral functions
bool do_fdc_init();
void identify_drives(uint8_t *typa, uint8_t *typb);
char *get_flptype_string(uint8_t flptype);
void init_drive(drive_t drv);
void set_motor_state(drive_t drive, bool state);
void select_drive(drive_t drive);
bool send_cmd(uint8_t cmd);
bool send_cmd_parameter(uint8_t byte);
bool read_data_byte(uint8_t *byte, uint32_t timeout);
dor_t read_dor();
void write_dor(dor_t val);
msr_t read_msr();
bool cmd_version(uint8_t *ver);
bool cmd_configure(bool implseek_en, bool fifo_en, bool poll_en,
                   uint8_t threashold, uint8_t precomp);
bool cmd_set_lock(bool lock);
void reset();

void fdc_init()
{

    // Identify drives
    uint8_t typa, typb;
    identify_drives(&typa, &typb);
    kprintf("[FDC] Drive A: %s, Drive B: %s\n",
            get_flptype_string(typa), get_flptype_string(typb));

    // Initialize controller
    if (!do_fdc_init())
    {
        kprintf("[FDC] Error initializing FDC!\n");
        return;
    }

    // Iniitalize drives
    if (typa == CMOS_FLPTYPE_35_144M)
        init_drive(DRIVE_A);
    if (typb == CMOS_FLPTYPE_35_144M)
        init_drive(DRIVE_B);
}

/* Internal functions */

// Perform FDC initialization sequence
bool do_fdc_init()
{
    for (uint32_t i = 0; i < CMD_RETRIES; i++)
    {
        // Read version
        uint8_t ver;
        if (!cmd_version(&ver))
            continue;

#ifdef DEBUG
        kprintf("[FDC] Version: 0x%x\n", ver);
#endif

        // Check version
        if (ver != 0x80 && ver != 0x90)
            return false;

        // Configure
        if (!cmd_configure(true, true, false, 8, 0))
            return false;

        // Lock configuration
        if (!cmd_set_lock(true))
            return false;

        // Reset controller
        reset();

        kprintf("[FDC] MSR: 0x%x\n", read_msr().bits);

        return true;
    }
    return false;
}

// Read and parse CMOS floppy drive info
void identify_drives(uint8_t *typa, uint8_t *typb)
{
    // Read CMOS floppy register
    uint8_t reg = cmos_read_reg(CMOS_REG_FLPTYPE);

    *typa = reg >> 4;
    *typb = reg & 0xF;
}

char *get_flptype_string(uint8_t flptype)
{
    switch (flptype)
    {
    // case CMOS_FLPTYPE_NONE:
    //     return "None";
    // case CMOS_FLPTYPE_525_360K:
    //     return "5.25 360K";
    // case CMOS_FLPTYPE_525_12M:
    //     return "5.25 1.2M";
    // case CMOS_FLPTYPE_35_720K:
    //     return "3.5 720K";
    case CMOS_FLPTYPE_35_144M:
        return "3.5 1.44M";
        // case CMOS_FLPTYPE_35_288M:
        //     return "3.5 2.88M";
    }
    return "None";
}

void init_drive(drive_t drv)
{
#ifdef DEBUG
    kprintf("[FDC] Initializing drive %c\n", drv);
#endif
}

// Set motor state for a drive
void set_motor_state(drive_t drive, bool state)
{
    dor_t val = read_dor();

    switch (drive)
    {
    case DRIVE_A:
        val.mota = state;
        break;
    case DRIVE_B:
        val.motb = state;
        break;
    default:
        return;
    }

    write_dor(val);
}

void select_drive(drive_t drive)
{
    // Look up drive bits
    uint8_t dsel;
    switch (drive)
    {
    case DRIVE_A:
        dsel = 0;
        break;
    case DRIVE_B:
        dsel = 1;
        break;
    default:
        return;
    }

    dor_t val = read_dor();
    val.dsel = dsel;
    write_dor(val);
}

dor_t read_dor()
{
    dor_t val;
    val.bits = inb(PORT_DOR);
    return val;
}

void write_dor(dor_t val)
{
    return outb(PORT_DOR, val.bits);
}

msr_t read_msr()
{
    msr_t val;
    val.bits = inb(PORT_MSR);
    return val;
}

// Send command byte to FDC
// Returns false on failure
bool send_cmd(uint8_t cmd)
{
    msr_t msr = read_msr();

    // Check initial condition
    if (!msr.rqm || msr.dio)
        return false;

    // Send cmd byte
    outb(PORT_FIFO, cmd);

    return true;
}

// Send parameter byte to FDC
// Returns false on failure
bool send_cmd_parameter(uint8_t byte)
{
    uint32_t start = clock_get_system();

    msr_t msr;

    // Wait for FIFO to be ready
    while (!(msr = read_msr()).rqm)
    {
        pause();
        if (clock_get_system() - start > CMD_TIMEOUT)
            return false;
    }

    // Verify DIO
    if (msr.dio)
        return false;

    // Send byte
    outb(PORT_FIFO, byte);

    return true;
}

// Read data byte from FIFO
bool read_data_byte(uint8_t *byte, uint32_t timeout)
{

    uint32_t start = clock_get_system();

    msr_t msr;

    // Wait for FIFO to be ready
    while ((msr = read_msr()).rqm != 1)
    {
        pause();
        if (clock_get_system() - start > timeout)
            return false;
    }

    *byte = inb(PORT_FIFO);

    return true;
}

// Version command
bool cmd_version(uint8_t *ver)
{

    // Send command
    if (!send_cmd(CMD_VERSION))
        return false;

    // Read result
    if (!read_data_byte(ver, CMD_TIMEOUT))
        return false;

    return true;
}

// Configure command
bool cmd_configure(bool implseek_en, bool fifo_en, bool poll_en,
                   uint8_t threashold, uint8_t precomp)
{
    // Send command
    if (!send_cmd(CMD_CONFIGURE))
        return false;

    // Compute values
    uint8_t byte2 = (implseek_en << 6) | (!fifo_en << 5) |
                    (!poll_en << 4) | (threashold & 0x1F);

    // Send parameters
    if (!send_cmd_parameter(0) ||
        !send_cmd_parameter(byte2) ||
        !send_cmd_parameter(precomp))
        return false;

    return true;
}

// Lock command
bool cmd_set_lock(bool lock)
{
    uint8_t cmd = CMD_LOCK;

    // Set lock bit
    if (lock)
        cmd |= CMD_BIT_MT;

    // Send command
    if (!send_cmd(cmd))
        return false;

    // Read result
    uint8_t res;
    if (!read_data_byte(&res, CMD_TIMEOUT))
        return false;

    // Check result
    if (res != (lock << 4))
        return false;

    return true;
}

// Reset FDC
void reset()
{
    outb(PORT_DSR, 0x88);
}
