#include "drivers/kbdctl.h"

#include "sys/io.h"

#include "log.h"
#include "panic.h"
#include "clock.h"

#define PORT_DATA 0x60
#define PORT_CMD 0x64

#define KBDCTL_SELFTEST_OK 0x55
#define INT_TEST_OK 0x00

typedef enum
{
    CMD_READ_BYTE_0 = 0x20,
    CMD_WRITE_BYTE_0 = 0x60,
    CMD_DISABLE_PORT2 = 0xA7,
    CMD_ENABLE_PORT2 = 0xA8,
    CMD_TEST_PORT2 = 0xA9,
    CMD_TEST_KBDCTL = 0xAA,
    CMD_TEST_PORT1 = 0xAB,
    CMD_DISABLE_PORT1 = 0xAD,
    CMD_ENABLE_PORT1 = 0xAE,
    CMD_READ_OUTP = 0xD0,
    CMD_WRITE_OUTP = 0xD1,
    CMD_PULSE_OUTP0 = 0xF0,
} kbdctl_cmd;

// Keyboard controller configuration byte
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t port1_irq_en : 1;
        uint8_t port2_irq_en : 1;
        uint8_t post_success : 1;
        uint8_t _res0 : 1;
        uint8_t port1_clock_en : 1;
        uint8_t port2_clock_en : 1;
        uint8_t port1_trans_en : 1;
        uint8_t zero : 1;
    };
} kbdctl_cfg_byte;

// Keyboard controller status register
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        uint8_t outbuf_full : 1;
        uint8_t inbuf_full : 1;
        uint8_t system : 1;
        uint8_t cmd_data : 1;
        uint8_t _res0 : 1;
        uint8_t _res1 : 1;
        uint8_t timeout_err : 1;
        uint8_t parity_err : 1;
    };
} kbdctl_sreg;

// Internal function prototypes
static inline void kbdctl_send_cmd(kbdctl_cmd cmd);
static inline kbdctl_sreg kbdctl_read_sreg();
static inline void kbdctl_write_cfg_byte(kbdctl_cfg_byte cfg_byte);
static inline kbdctl_cfg_byte kbdctl_read_cfg_byte();
static inline bool kbdctl_selftest();
static inline void kbdctl_flush_outbuf();

/* Public functions */

void kbdctl_init()
{
    klog("[KBD] Initializing controller...\n");

    // Disable devices
    kbdctl_send_cmd(CMD_DISABLE_PORT1);
    kbdctl_send_cmd(CMD_DISABLE_PORT2);

    // Flush output buffer
    kbdctl_flush_outbuf();

    // Set configuration byte
    kbdctl_cfg_byte cfg_byte = kbdctl_read_cfg_byte();
    cfg_byte.port1_irq_en = 1;   // Disable interrupts (for now)
    cfg_byte.port1_clock_en = 1; // Make sure clock is enabled
    cfg_byte.port1_trans_en = 0; // Disable legacy translation
    kbdctl_write_cfg_byte(cfg_byte);

    // Controller self test
    if (!kbdctl_selftest())
        panic("KBDCTL_SELFTEST_FAIL", "Keyboard controller selftest failure");
}

uint8_t kbdctl_read_data()
{
    // Wait for data to be available
    while (!kbdctl_outbuf_full())
        ;

    return inb(PORT_DATA);
}

bool kbdctl_read_data_timeout(uint8_t *data, uint32_t timeout)
{
    uint32_t start = clock_get_system();

    // Wait for data to be available
    while (!kbdctl_outbuf_full())
    {
        if (clock_get_system() - start > timeout)
            return false;
    }

    // Read data
    *data = inb(PORT_DATA);

    return true;
}

bool kbdctl_outbuf_full()
{
    return kbdctl_read_sreg().outbuf_full;
}
// Perform interface test on port 1
bool kbdctl_test_port1()
{
    kbdctl_send_cmd(CMD_TEST_PORT1);

    // Read result
    uint8_t result = kbdctl_read_data();
    return result == INT_TEST_OK;
}

void kbdctl_enable_irqs()
{
    kbdctl_cfg_byte cfg_byte = kbdctl_read_cfg_byte();
    cfg_byte.port1_irq_en = 0; // Enable port 1 interrupts
    kbdctl_write_cfg_byte(cfg_byte);
}

void kbdctl_reset_cpu()
{
    kbdctl_send_cmd(CMD_PULSE_OUTP0);
}

/* Internal functions */

// Send command to keyboard controller
static inline void kbdctl_send_cmd(kbdctl_cmd cmd)
{
    outb(PORT_CMD, cmd);
}

// Read keyboard controller status register
static inline kbdctl_sreg kbdctl_read_sreg()
{
    kbdctl_sreg sreg;
    sreg.bits = inb(PORT_CMD);
    return sreg;
}

// Write keyboard controller configuration byte
static inline void kbdctl_write_cfg_byte(kbdctl_cfg_byte cfg_byte)
{
    outb(PORT_CMD, CMD_WRITE_BYTE_0);
    outb(PORT_DATA, cfg_byte.bits);
}

// Read keyboard controller configuration byte
static inline kbdctl_cfg_byte kbdctl_read_cfg_byte()
{
    outb(PORT_CMD, CMD_READ_BYTE_0);
    kbdctl_cfg_byte cfg_byte;
    cfg_byte.bits = inb(PORT_DATA);
    return cfg_byte;
}

// Perform keyboard controller self test
static inline bool kbdctl_selftest()
{
    // Start self test
    outb(PORT_CMD, CMD_TEST_KBDCTL);

    // Read result
    uint8_t result = kbdctl_read_data();
    return result == KBDCTL_SELFTEST_OK;
}

// Flush the keyboard controller output buffer
static inline void kbdctl_flush_outbuf()
{
    while (kbdctl_outbuf_full())
        kbdctl_read_data();
}
