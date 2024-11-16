#include "drivers/fdc.h"

#include <stdint.h>
#include "sync.h"
#include "sys/io.h"
#include "mini-printf.h"

#include "config.h"
#include "mem/kalloc.h"
#include "log.h"
#include "drivers/cmos.h"
#include "clock.h"
#include "cpu.h"
#include "int/interrupts.h"
#include "clock.h"
#include "blkdev/blkdev.h"

// Configure debugging
#if DEBUG_FDC == 1
#define DEBUG
#endif

#define CMD_TIMEOUT 100
#define CMD_RETRIES 3
#define RW_TIMEOUT 5000
#define RW_RETRIES 5
#define MOTOR_OFF_DELAY 2000
#define MOTOR_SPINUP_TIME 300

// Geometry
#define CYLS 80
#define HEADS 2
#define SECTORS 18
#define SECTOR_SIZE 512 // bytes

#define SRT 8 // 4ms
#define HLT 5 // 10ms
#define HUT 0 // Maximum

#define FLOPPY_IRQ 6

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

#define DATARATE_500KBPS 0

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
        uint8_t not_reset : 1;
        uint8_t irq : 1;
        uint8_t mota : 1;
        uint8_t motb : 1;
        uint8_t motc : 1;
        uint8_t motd : 1;
    };
} dor_t;

// ST0 bits
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t ds : 2; // Selected drive
        uint8_t h : 1;  // Head address
        uint8_t _res0 : 1;
        uint8_t ec : 1; // Equipment check (Recalibrate failure)
        uint8_t se : 1; // Seek end (asserted after seek)
        uint8_t ic : 2; // Interrupt code (command result)
    };
} st0_t;

#define ST0_IC_SUCC 0x0    // Operation succesful
#define ST0_IC_NOTCOMP 0x1 // Operation not completed
#define ST0_IC_INVCMD 0x2  // Invalid command
#define ST0_IC_POLL 0x3    // Opeation terminated because of polling

// ST1 bits
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t ma : 1; // Missing address mark
        uint8_t nw : 1; // Not writeable
        uint8_t nd : 1; // No data
        uint8_t _res0 : 1;
        uint8_t or : 1; // Overrun/underrun
        uint8_t de : 1; // CRCerror
        uint8_t _res1 : 1;
        uint8_t en : 1; // End of cylinder
    };
} st1_t;

// ST2 bits
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t _res0 : 1;
        uint8_t cm : 1; // Control mark
        uint8_t dd : 1; // Data error
        uint8_t wc : 1; // Wrong cylinder
        uint8_t _res1 : 1;
        uint8_t _res2 : 1;
        uint8_t bc : 1; // Bad cylinder
        uint8_t md : 1; // Missin Data address mark
    };
} st2_t;

// Drives
typedef enum
{
    DRIVE_0 = 0,
    DRIVE_1 = 1,
} drive_t;

// Drive state state
// Is the private state inside the block device struct
typedef struct
{
    drive_t drive;
    slock_t drv_lck;

    bool motor_on;
    timer_handle_t motor_timer;
    bool has_motor_timer;

    bool media_changed;

} fdc_drv_state_t;

// Intenral functions
static bool do_fdc_init();
static void identify_drives(uint8_t *typa, uint8_t *typb);
static char *get_flptype_string(uint8_t flptype);
static void init_drive(drive_t drv);
static void set_motor_state(drive_t drive, bool state);
static void select_drive(drive_t drive);
// bool send_cmd(uint8_t cmd);
static bool send_byte(uint8_t byte);
static bool read_data_byte(uint8_t *byte, uint32_t timeout);
static dor_t read_dor();
static void write_dor(dor_t val);
static msr_t read_msr();
static void write_dsr(uint8_t val);
static uint8_t read_dir();
static bool check_media_changed(drive_t drive);
static bool cmd_version(uint8_t *ver);
static bool cmd_configure(bool implseek_en, bool fifo_en, bool poll_en,
                          uint8_t threashold, uint8_t precomp);
static bool cmd_set_lock(bool lock);
static bool cmd_sense_interrupt(st0_t *st0, uint8_t *cyl);
static bool cmd_specify(uint8_t srt, uint8_t hut, uint8_t hlt, bool pio_mode);
static bool cmd_recalibrate(drive_t drive);
static bool cmd_seek(drive_t drive, uint8_t cyl);
static bool cmd_read_sector(uint8_t *buf, drive_t drive, uint8_t cyl, uint8_t head,
                            uint8_t sect);
static bool reset();
static bool read_req(blkdev_t *dev, uint8_t *buf, uint32_t block);
static bool write_req(blkdev_t *dev, uint8_t *buf, uint32_t block);
static bool media_changed_req(blkdev_t *dev);
static bool access_drive(fdc_drv_state_t *state);
static void unaccess_drive(fdc_drv_state_t *state);
static void destroy_state(fdc_drv_state_t *state);
static bool wait_irq6_timeout(uint32_t timeout);
static inline void lba_to_chs(uint8_t *c, uint8_t *h, uint8_t *s,
                              uint32_t lba);
void irq6_handler();
void motor_off_cb(void *data);

// Global objects
bool irq6_received;

void fdc_init()
{
    // Register IRQ
    interrupts_register_irq(FLOPPY_IRQ, irq6_handler);

    // Initialize controller
    if (!do_fdc_init())
    {
#ifdef DEBUG
        kprintf("[FDC] Error initializing FDC\n");
#endif
        interrupts_unregister_irq(FLOPPY_IRQ, irq6_handler);
        return;
    }

    // Identify drives using CMOS
    uint8_t typ0, typ1;
    identify_drives(&typ0, &typ1);
    kprintf("[FDC] Drive 0: %s, Drive 1: %s\n",
            get_flptype_string(typ0), get_flptype_string(typ1));

    // Iniitalize drives
    if (typ0 == CMOS_FLPTYPE_35_144M)
        init_drive(DRIVE_0);
    if (typ1 == CMOS_FLPTYPE_35_144M)
        init_drive(DRIVE_1);
}

/* Internal functions */

// Perform FDC initialization sequence
static bool do_fdc_init()
{
    for (uint32_t i = 0; i < CMD_RETRIES; i++)
    {
        // Enable IRQ
        dor_t dor = read_dor();
        dor.irq = 1;
        write_dor(dor);

        // Reset controller
        if (!reset())
            continue;

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

        // Unlock configuration
        if (!cmd_set_lock(false))
            continue;

        // Configure
        if (!cmd_configure(true, true, false, 8, 0))
            continue;

        // Lock configuration
        if (!cmd_set_lock(true))
            continue;

        // Initialization successful
        return true;
    }
    return false;
}

// Read and parse CMOS floppy drive info
static void identify_drives(uint8_t *typa, uint8_t *typb)
{
    // Read CMOS floppy register
    uint8_t reg = cmos_read_reg(CMOS_REG_FLPTYPE);

    *typa = reg >> 4;
    *typb = reg & 0xF;
}

static void init_drive(drive_t drv)
{
#ifdef DEBUG
    char *failure = NULL;
    kprintf("[FDC] Initializing drive %d\n", drv);
#endif

    // Allocate drive state
    fdc_drv_state_t *state = kalloc(sizeof(fdc_drv_state_t));
    if (!state)
    {
#ifdef DEBUG
        failure = "out of memory";
#endif
        goto fail_nomem_state;
    }

    // Set up state
    slock_init(&state->drv_lck);
    state->drive = drv;
    state->has_motor_timer = false;
    state->motor_on = false;
    state->media_changed = false;

    slock_acquire(&state->drv_lck);

    bool succ = false;
    for (int i = 0; i < RW_RETRIES; i++)
    {
        // Set up FDC for access to this drive
        if (!access_drive(state))
            goto reset;

        // Recalibrate
        if (!cmd_recalibrate(state->drive))
            goto reset;

        succ = true;
        break;
    reset:
        reset();
    }

    // kprintf("Initialized\n");

    // clock_delay_ms(1000);

    // // Seek to track 79
    // if (!cmd_seek(drv, 79))
    //     kprintf("Seek error!\n");

    // kprintf("Seek OK!\n");

    // clock_delay_ms(1000);

    // // Seek back home
    // if (!cmd_seek(drv, 0))
    //     kprintf("Seek error!\n");

    // kprintf("Seek OK!\n");

    unaccess_drive(state);

    slock_release(&state->drv_lck);

    // Check if initialization was succesful
    if (!succ)
    {
#ifdef DEBUG
        failure = "drive communication error";
#endif
        goto fail_drvfail;
    }

    // Drive is correctly set up, register it with the block device subsytem

    // Construct major
    size_t major_len = snprintf(NULL, 0, "fd%d", drv) + 1;
    char *major = kalloc(major_len);
    if (!major)
    {
#ifdef DEBUG
        failure = "out of memory";
#endif
        goto fail_nomem_major;
    }
    snprintf(major, major_len, "fd%d", drv);

    // Set up block device
    blkdev_t blkdev = {
        .major = major,
        .nblocks = CYLS * HEADS * SECTORS,
        .drvstate = state,
        .read_blk = read_req,
        .write_blk = write_req,
        .media_changed = media_changed_req,
    };

    // Register block device
    if (!blkdev_register(blkdev))
    {
#ifdef DEBUG
        failure = "unable to register block device";
#endif
        goto fail_blkdevreg;
    }

    goto success;

    // Handle deallocating memory on failure
fail_blkdevreg:
    // Deallocate major string
    kfree(major);
fail_nomem_major:
fail_drvfail:
    // Get drive in a quiet state, then deallocate the state object
    destroy_state(state);
fail_nomem_state:

#ifdef DEBUG
    kprintf("[FDC] Drive %d initialization error: %s\n", drv, failure);
#endif
success:
}

// Set motor state for a drive
static void set_motor_state(drive_t drive, bool state)
{
    dor_t val = read_dor();

    switch (drive)
    {
    case DRIVE_0:
        val.mota = state;
        break;
    case DRIVE_1:
        val.motb = state;
        break;
    default:
        return;
    }

    write_dor(val);
}

// Select drive in DOR
static void select_drive(drive_t drive)
{
    // Look up drive bits
    uint8_t dsel;
    switch (drive)
    {
    case DRIVE_0:
        dsel = 0;
        break;
    case DRIVE_1:
        dsel = 1;
        break;
    default:
        return;
    }

    dor_t val = read_dor();
    val.dsel = dsel;
    write_dor(val);
}

static dor_t read_dor()
{
    dor_t val;
    val.bits = inb(PORT_DOR);
    io_delay();
    return val;
}

static void write_dor(dor_t val)
{
    outb(PORT_DOR, val.bits);
    io_delay();
}

static msr_t read_msr()
{
    msr_t val;
    val.bits = inb(PORT_MSR);
    io_delay();
    return val;
}

static void write_dsr(uint8_t val)
{
    outb(PORT_DSR, val);
    io_delay();
}

static uint8_t read_dir()
{
    uint8_t val = inb(PORT_DIR);
    io_delay();
    return val;
}

static bool check_media_changed(drive_t drive)
{
    // Check bit
    return read_dir() & (1 << 8) != 0;
}

// Send parameter byte to FDC
// Returns false on failure
static bool send_byte(uint8_t byte)
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
    io_delay();

    return true;
}

// Read data byte from FIFO
static bool read_data_byte(uint8_t *byte, uint32_t timeout)
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
    io_delay();

    return true;
}

// Version command
static bool cmd_version(uint8_t *ver)
{

    // Send command
    if (!send_byte(CMD_VERSION))
    {
        kprintf("[FDC] Error sending version command\n");
        return false;
    }

    // Read result
    if (!read_data_byte(ver, CMD_TIMEOUT))
    {
        kprintf("[FDC] Error reading data\n");
        return false;
    }

    return true;
}

// Configure command
static bool cmd_configure(bool implseek_en, bool fifo_en, bool poll_en,
                          uint8_t threashold, uint8_t precomp)
{
    // Send command
    if (!send_byte(CMD_CONFIGURE))
        return false;

    // Compute values
    uint8_t byte2 = (implseek_en << 6) | (!fifo_en << 5) |
                    (!poll_en << 4) | (threashold & 0x1F);

    // Send parameters
    if (!send_byte(0) ||
        !send_byte(byte2) ||
        !send_byte(precomp))
        return false;

    return true;
}

// Lock command
static bool cmd_set_lock(bool lock)
{
    uint8_t cmd = CMD_LOCK;

    // Set lock bit
    if (lock)
        cmd |= CMD_BIT_MT;

    // Send command
    if (!send_byte(cmd))
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

// Specify command
static bool cmd_specify(uint8_t srt, uint8_t hut, uint8_t hlt, bool pio_mode)
{
    // Send command
    if (!send_byte(CMD_SPECIFY))
        return false;

    // Compute values
    uint8_t byte0 = (srt << 4) | (hut & 0xF);
    uint8_t byte1 = (hlt << 1) | (pio_mode & 0x1);

    // Send parameters
    if (!send_byte(byte0) ||
        !send_byte(byte1))
        return false;

    return true;
}

// Recalibrate command
static bool cmd_recalibrate(drive_t drive)
{
    irq6_received = false;

    // Send command
    if (!send_byte(CMD_RECALIBRATE))
        return false;

    // Send parameters
    if (!send_byte(drive))
        return false;

    // Wait for completion
    if (!wait_irq6_timeout(RW_TIMEOUT))
        return false;

    // Read result
    st0_t st0;
    uint8_t cyl;
    if (!cmd_sense_interrupt(&st0, &cyl))
        return false;

#ifdef DEBUG
    kprintf("[FDC] Recalibrate result:\n");
    kprintf("ST0: IC=%d, SE=%d, EC=%d, H=%d, DS=%x\n", st0.ic, st0.se, st0.ec, st0.h, st0.ds);
    kprintf("Cylinder: %d\n", cyl);
#endif

    return st0.ic == ST0_IC_SUCC && st0.se && cyl == 0;
}

// Seek command
static bool cmd_seek(drive_t drive, uint8_t cyl)
{
    irq6_received = false;

    // Send command
    if (!send_byte(CMD_SEEK))
        return false;

    // Send parameters
    if (!send_byte(drive) ||
        !send_byte(cyl))
        return false;

    // Wait for completion
    if (!wait_irq6_timeout(RW_TIMEOUT))
        return false;

    // Read result
    st0_t st0;
    uint8_t rescyl;
    if (!cmd_sense_interrupt(&st0, &rescyl))
        return false;

    return st0.ic == ST0_IC_SUCC && st0.se && rescyl == cyl;
}

static bool cmd_sense_interrupt(st0_t *st0, uint8_t *cyl)
{
    if (!send_byte(CMD_SENSE_INTERRUPT))
        return false;

    if (!read_data_byte(&st0->bits, CMD_TIMEOUT) ||
        !read_data_byte(cyl, CMD_TIMEOUT))
        return false;

    return true;
}

// Read sector from the floppy disk
// buf: buffer to read into
static bool cmd_read_sector(uint8_t *buf, drive_t drive, uint8_t cyl, uint8_t head,
                            uint8_t sect)
{

    uint8_t cmd_byte = CMD_READ_DATA | CMD_BIT_MF;

    // Send command
    if (!send_byte(cmd_byte))
        return false;

    uint8_t byte0 = (head << 2) | drive;
    uint8_t byte1 = cyl;
    uint8_t byte2 = head;
    uint8_t byte3 = sect;
    uint8_t byte4 = 2;    // Hard coded for 512 byte sectors
    uint8_t byte5 = sect; // End reading at the same sector we started
    uint8_t byte6 = 0x1B; // GAP1 size
    uint8_t byte7 = 0xFF; // Unused as byte 4 is set != 0

    // Send parameters
    if (!send_byte(byte0) ||
        !send_byte(byte1) ||
        !send_byte(byte2) ||
        !send_byte(byte3) ||
        !send_byte(byte4) ||
        !send_byte(byte5) ||
        !send_byte(byte6) ||
        !send_byte(byte7))
        return false;

    // Read data
    uint32_t bytes_read = 0;
    while (bytes_read < SECTOR_SIZE)
    {
        msr_t msr;
        // Wait for data available
        while (!(msr = read_msr()).rqm)
            pause();

        // Execution phase terminated early
        // or wrong data direction
        if (!msr.ndma || !msr.dio)
            break;

        // Read byte
        buf[bytes_read] = inb(PORT_FIFO);
        // Don't need io delay

        bytes_read++;
    }

    //// Wait for buffer overrun
    // while (true)
    // {
    //     msr_t msr = read_msr();
    //     if (!msr.ndma || !msr.busy)
    //         break;
    // }

    // Read result bytes
    st0_t st0;
    st1_t st1;
    st2_t st2;
    uint8_t endcyl, endhead, endsect, two;
    if (!read_data_byte(&st0.bits, CMD_TIMEOUT) ||
        !read_data_byte(&st1.bits, CMD_TIMEOUT) ||
        !read_data_byte(&st2.bits, CMD_TIMEOUT) ||
        !read_data_byte(&endcyl, CMD_TIMEOUT) ||
        !read_data_byte(&endhead, CMD_TIMEOUT) ||
        !read_data_byte(&endsect, CMD_TIMEOUT) ||
        !read_data_byte(&two, CMD_TIMEOUT))
        return false;

#ifdef DEBUG
    kprintf("[FDC] Read result:\n");
    kprintf("ST0: IC=%d, SE=%d, EC=%d, H=%d, DS=%x\n", st0.ic, st0.se, st0.ec, st0.h, st0.ds);
    kprintf("ST1: EN=%d, DE=%d, OR=%d, ND=%d, NW=%d, MA=%d\n", st1.en, st1.de, st1.or, st1.nd, st1.nd, st1.nw, st1.ma);
    kprintf("ST2: MD=%d, BC=%d, WC=%d, DD=%d, CM=%d\n", st2.md, st2.bc, st2.wc, st2.dd, st2.cm);
    kprintf("endcyl: 0x%x, endhead: 0x%x, endsect: 0x%x\n", (int)endcyl, (int)endhead, (int)endsect);
#endif

    // Check command result
    if (st0.ic != ST0_IC_SUCC)
        return false;

    // Check number of bytes read
    if (bytes_read != SECTOR_SIZE)
        return false;

    return true;
}

// Write sector to the floppy disk
// buf: buffer to write from
// TODO: writes don't work on QEMU
static bool cmd_write_sector(uint8_t *buf, drive_t drive, uint8_t cyl, uint8_t head,
                             uint8_t sect)
{

    uint8_t cmd_byte = CMD_WRITE_DATA | CMD_BIT_MF;

    // Send command
    if (!send_byte(cmd_byte))
        return false;

    uint8_t byte0 = (head << 2) | drive;
    uint8_t byte1 = cyl;
    uint8_t byte2 = head;
    uint8_t byte3 = sect;
    uint8_t byte4 = 2;    // Hard coded for 512 byte sectors
    uint8_t byte5 = sect; // End reading at the same sector we started
    uint8_t byte6 = 0x1B; // GAP1 size
    uint8_t byte7 = 0xFF; // Unused as byte 4 is set != 0

    // Send parameters
    if (!send_byte(byte0) ||
        !send_byte(byte1) ||
        !send_byte(byte2) ||
        !send_byte(byte3) ||
        !send_byte(byte4) ||
        !send_byte(byte5) ||
        !send_byte(byte6) ||
        !send_byte(byte7))
        return false;

    // Read data
    uint32_t bytes_written = 0;
    while (bytes_written < SECTOR_SIZE)
    {
        msr_t msr;
        // Wait for data available
        while (!(msr = read_msr()).rqm)
            pause();

        // Execution phase terminated early
        // Or wrong data direction
        if (!msr.ndma || msr.dio)
            break;

        // Read byte
        outb(PORT_FIFO, buf[bytes_written]);
        // Don't need io delay

        bytes_written++;
    }

    // Read result bytes
    st0_t st0;
    st1_t st1;
    st2_t st2;
    uint8_t endcyl, endhead, endsect, two;
    if (!read_data_byte(&st0.bits, CMD_TIMEOUT) ||
        !read_data_byte(&st1.bits, CMD_TIMEOUT) ||
        !read_data_byte(&st2.bits, CMD_TIMEOUT) ||
        !read_data_byte(&endcyl, CMD_TIMEOUT) ||
        !read_data_byte(&endhead, CMD_TIMEOUT) ||
        !read_data_byte(&endsect, CMD_TIMEOUT) ||
        !read_data_byte(&two, CMD_TIMEOUT))
        return false;

#ifdef DEBUG
    kprintf("[FDC] Write result:\n");
    kprintf("ST0: IC=%d, SE=%d, EC=%d, H=%d, DS=%x\n", st0.ic, st0.se, st0.ec, st0.h, st0.ds);
    kprintf("ST1: EN=%d, DE=%d, OR=%d, ND=%d, NW=%d, MA=%d\n", st1.en, st1.de, st1.or, st1.nd, st1.nd, st1.nw, st1.ma);
    kprintf("ST2: MD=%d, BC=%d, WC=%d, DD=%d, CM=%d\n", st2.md, st2.bc, st2.wc, st2.dd, st2.cm);
    kprintf("endcyl: 0x%x, endhead: 0x%x, endsect: 0x%x\n", (int)endcyl, (int)endhead, (int)endsect);
#endif

    // Check command result
    if (st0.ic != ST0_IC_SUCC)
        return false;

    // Check number of bytes read
    if (bytes_written != SECTOR_SIZE)
        return false;

    return true;
}

// Reset FDC
static bool reset()
{
    irq6_received = false;

    dor_t dor = read_dor();

    // Disable controller
    dor.not_reset = 0;
    write_dor(dor);

    // Few microsecond delay
    for (int i = 0; i < 10; i++)
        io_delay();

    // Enable controller
    dor.not_reset = 1;
    write_dor(dor);

    // Set data rate
    // write_dsr(DATARATE_500KBPS);

    // Wait for ir6
    if (!wait_irq6_timeout(1000))
        return false;

    st0_t st0;
    uint8_t cyl;
    for (int i = 0; i < 4; i++)
    {

        if (!cmd_sense_interrupt(&st0, &cyl))
            return false;
    }

    // kprintf("Reset: ST0 = 0x%x, CYL = 0x%x\n", st0, cyl);

    return true;
}

// Set up controller for accessing a drive
// Returns false on failure
static bool access_drive(fdc_drv_state_t *state)
{
    // Select the drive
    select_drive(state->drive);

    // Turn on motor
    set_motor_state(state->drive, true);

    // Set data rate
    write_dsr(DATARATE_500KBPS);

    // Wait for spinup if motor was not already on
    // if (!state->motor_on)
    //     clock_delay_ms(MOTOR_SPINUP_TIME);
    state->motor_on = true;

    // Send specify (Enable PIO mode)
    if (!cmd_specify(SRT, HUT, HLT, true))
    {
#ifdef DEBUG
        kprintf("[FDC] Specify error\n");
#endif
        return false;
    }

    return true;
}

// Free drive
// Sets up the motor off timer
static void unaccess_drive(fdc_drv_state_t *state)
{
    // If no timer was set or the timer could not be reset
    if (!state->has_motor_timer ||
        !clock_reset_timer(state->motor_timer, MOTOR_OFF_DELAY))
    {
        // Set new timer
        state->motor_timer = clock_set_timer(MOTOR_OFF_DELAY, TIMER_ONESHOT, motor_off_cb, state);
        state->has_motor_timer = true;

        // Check timer was set correctly, else turn off motor now
        if (state->motor_timer < 0)
        {
            set_motor_state(state->drive, false);
            state->motor_on = false;
            state->has_motor_timer = false;
        }
    }
}

// Block device read operation
static bool read_req(blkdev_t *dev, uint8_t *buf, uint32_t block)
{

    fdc_drv_state_t *state = (fdc_drv_state_t *)dev->drvstate;

#ifdef DEBUG
    kprintf("[FDC] Drive %d read block %d\n", state->drive, block);
#endif

    // Acquire drive lock
    slock_acquire(&state->drv_lck);

    // Convert block address to chs
    uint8_t cyl, head, sect;
    lba_to_chs(&cyl, &head, &sect, block);

    // Retry command many times
    bool read_succ = false;
    for (int i = 0; i < RW_RETRIES; i++)
    {
        // Set up drive for access
        if (!access_drive(state))
            goto reset;

        // Set up datarate
        write_dsr(DATARATE_500KBPS);

        // Check media changed
        if (check_media_changed(state->drive))
            state->media_changed = true;

        // Seek to cylinder
        if (!cmd_seek(state->drive, cyl))
            goto reset;
        // NOTE: we don't need to seek explicitly thanks to implied seek

        // Do read
        if (!cmd_read_sector(buf, state->drive, cyl, head, sect))
            goto reset;

        read_succ = true;
        break;
    reset:
        reset();
    }

    // Set up motor off timer
    unaccess_drive(state);

    // Release drive lock
    slock_release(&state->drv_lck);

    return read_succ;
}

// Block device write operation
static bool write_req(blkdev_t *dev, uint8_t *buf, uint32_t block)
{
    fdc_drv_state_t *state = (fdc_drv_state_t *)dev->drvstate;

#ifdef DEBUG
    kprintf("[FDC] Drive %d write block %d\n", state->drive, block);
#endif

    // Acquire drive lock
    slock_acquire(&state->drv_lck);

    // Convert block address to chs
    uint8_t cyl, head, sect;
    lba_to_chs(&cyl, &head, &sect, block);

    // Retry command many times
    bool write_succ = false;
    for (int i = 0; i < RW_RETRIES; i++)
    {
        // Set up drive for access
        if (!access_drive(state))
            goto reset;

        // Check media changed
        if (check_media_changed(state->drive))
            state->media_changed = true;

        // // Seek to cylinder
        // if (!cmd_seek(state->drive, 0))
        //     goto reset;
        // NOTE: we don't need to seek explicitly thanks to implied seek

        // Do read
        if (!cmd_write_sector(buf, state->drive, cyl, head, sect))
            goto reset;

        write_succ = true;
        break;
    reset:
        reset();
    }

    // Set up motor off timer
    unaccess_drive(state);

    // Release drive lock
    slock_release(&state->drv_lck);

    return write_succ;
}

static bool media_changed_req(blkdev_t *dev)
{
    fdc_drv_state_t *state = (fdc_drv_state_t *)dev->drvstate;

    // Acquire drive lock
    slock_acquire(&state->drv_lck);

    // Select drive
    select_drive(state->drive);

    // Check media changed
    if (check_media_changed(state->drive))
        state->media_changed = true;

    uint8_t res = state->media_changed;
    state->media_changed = false;

    // Release drive lock
    slock_release(&state->drv_lck);

    return res;
}

// Convert LBA addres to CHS
static inline void lba_to_chs(uint8_t *c, uint8_t *h, uint8_t *s,
                              uint32_t lba)
{
    *c = lba / (HEADS * SECTORS);
    *h = ((lba % (HEADS * SECTORS)) / SECTORS);
    *s = ((lba % (HEADS * SECTORS)) % SECTORS + 1);
}

// Destroy state, handling all potential cases
static void destroy_state(fdc_drv_state_t *state)
{
    // Acquire device lock
    slock_acquire(&state->drv_lck);

    // Delete motor timer
    if (state->has_motor_timer)
        clock_clear_timer(state->motor_timer);

    // Stop motor
    set_motor_state(state->drive, false);

    // Deallocate state
    kfree(state);
}

bool wait_irq6_timeout(uint32_t timeout)
{
    uint32_t start = clock_get_system();

    // Wait for IRQ6 to be called
    while (!irq6_received)
    {
        pause();
        if (clock_get_system() - start > timeout)
            return false;
    }

    return true;
}

void irq6_handler()
{
    irq6_received = true;
    // kprintf("IRQ6");
}

// Motor OFF callback
// Called by the clock subsystem inside the timer interrupt handler
void motor_off_cb(void *data)
{
    fdc_drv_state_t *state = (fdc_drv_state_t *)data;

    // Try obtaining drive lock
    if (!slock_try_acquire(&state->drv_lck))
        return;

#ifdef DEBUG
    kprintf("[FDC] Drive %d motor off\n", state->drive);
#endif

    // Turn off motor
    if (state->motor_on)
    {
        set_motor_state(state->drive, false);
        state->motor_on = false;
    }

    // Release drive lock
    slock_release(&state->drv_lck);
}

static char *get_flptype_string(uint8_t flptype)
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
