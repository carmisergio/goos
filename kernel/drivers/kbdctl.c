#include "drivers/kbdctl.h"

#include "sys/io.h"

#include "log.h"
#include "panic.h"
#include "clock.h"
#include "cpu.h"
#include "int/interrupts.h"

// Hardware specifications
#define PORT_DATA 0x60
#define PORT_CMD 0x64
#define IRQ_PORT1 1
#define IRQ_PORT2 12

// PS2 commands
#define PS2_RESET 0xFF
#define PS2_ACK 0xFA
#define PS2_SELFTEST_OK 0xAA
#define PS2_RESEND 0xFE

#define TIMEOUT 100 // (ms)

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
    CMD_WRITE_PORT2 = 0xD4,
    CMD_PULSE_OUTP0 = 0xF0,
} kbdctl_cmd;

typedef enum
{
    RES_SELFTEST_OK = 0x55,
    RES_INT_TEST_OK = 0x00,
} kbdctl_response;

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
        uint8_t port1_clock_dis : 1;
        uint8_t port2_clock_dis : 1;
        uint8_t port1_trans_en : 1;
        uint8_t zero : 1;
    };
} kbdctl_cfg_byte;

// Types of PS/2 devices
typedef enum
{
    KEYBOARD,
    MOUSE,
    UNKNOWN,
} device_type;

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

// Keybord controller ports
typedef enum
{
    PORT_1 = 1,
    PORT_2 = 2,
} kbdctl_port;

// Internal function prototypes
static inline void
kbdctl_send_cmd(kbdctl_cmd cmd);
static inline kbdctl_sreg kbdctl_read_sreg();
static inline void kbdctl_write_cfg_byte(kbdctl_cfg_byte cfg_byte);
static inline kbdctl_cfg_byte kbdctl_read_cfg_byte();
static inline bool kbdctl_selftest();
static inline void kbdctl_flush_outbuf();
static inline bool kbdctl_outbuf_full();
static inline bool kbdctl_inbuf_full();
static inline uint8_t kbdctl_read_data();
bool kbdctl_read_data_timeout(uint8_t *data, uint32_t timeout);
static inline void kbdctl_write_data(uint8_t data, kbdctl_port port);
static bool write_data_resend(uint8_t byte, kbdctl_port port);
static bool device_initialize(kbdctl_port port);
static device_type identify_device();
static void kbdctl_irq_port1();
static void kbdctl_irq_port2();

// // Global objects
// port_out_buf port1_out_buf, port2_out_buf;

/* Public functions */

void kbdctl_init()
{
    kbdctl_cfg_byte cfg_byte;
    bool use_port_1 = true; // Port 1 is present on all controllers
    bool use_port_2 = false;

    klog("[KBDCTL] Initializing controller...\n");

    // Disable devices
    kbdctl_send_cmd(CMD_DISABLE_PORT1);
    kbdctl_send_cmd(CMD_DISABLE_PORT2);

    // Flush output buffer
    kbdctl_flush_outbuf();

    // Disable legacy scancode translation
    cfg_byte = kbdctl_read_cfg_byte();
    cfg_byte.port1_irq_en = 0;
    cfg_byte.port2_irq_en = 0;
    cfg_byte.port1_trans_en = 0;
    kbdctl_write_cfg_byte(cfg_byte);

    // Controller self test
    if (!kbdctl_selftest())
    {
        kdbg("[KBDCTL] Selftest failure!");

        // Exit from driver
        return;
    }

    // Self test may reset controller configuration,
    // make sure its set correctly
    kbdctl_write_cfg_byte(cfg_byte);

    // Check if the controller has two ports
    kbdctl_send_cmd(CMD_ENABLE_PORT2);
    cfg_byte = kbdctl_read_cfg_byte();
    if (cfg_byte.port2_clock_dis == 0)
    {
        // Controller has two ports
        use_port_2 = true;

        // Disable second port again
        kbdctl_send_cmd(CMD_DISABLE_PORT2);
    }

    // Perform interface checks
    kbdctl_send_cmd(CMD_TEST_PORT1);
    if (kbdctl_read_data() != RES_INT_TEST_OK)
    {
        klog("Port 1 interface test failed!\n");
        // Disable port 1
        use_port_1 = false;
    }
    if (use_port_2)
    {
        kbdctl_send_cmd(CMD_TEST_PORT2);
        if (kbdctl_read_data() != RES_INT_TEST_OK)
        {
            klog("Port 2 interface test failed!\n");
            // Disable port 2
            use_port_2 = false;
        }
    }

    cfg_byte.port1_irq_en = 0;
    cfg_byte.port2_irq_en = 0;
    kbdctl_write_cfg_byte(cfg_byte);

    // kbdctl_send_cmd(CMD_DISABLE_PORT1);
    // Reset device 1
    if (use_port_1)
    {
        kbdctl_send_cmd(CMD_ENABLE_PORT1);

        use_port_1 = device_initialize(PORT_1);

        // We disable the port again to distinguish between messages coming
        // from the two ports during initialization
        kbdctl_send_cmd(CMD_DISABLE_PORT1);
    }

    // Reset device 2
    if (use_port_2)
    {
        kbdctl_send_cmd(CMD_ENABLE_PORT2);

        use_port_2 = device_initialize(PORT_2);

        // We disable the port again to distinguish between messages coming
        // from the two ports during initialization
        kbdctl_send_cmd(CMD_DISABLE_PORT2);
    }

    if (use_port_1)
        klog("Detected device 1!\n");
    if (use_port_2)
        klog("Detected device 2!\n");

    if (use_port_1)
        write_data_resend(0xF4, PORT_1);
    if (use_port_2)
        write_data_resend(0xF4, PORT_2);

    // Enable interupts
    cfg_byte.port1_irq_en = 1;
    cfg_byte.port2_irq_en = 1;
    kbdctl_write_cfg_byte(cfg_byte);

    // Enable ports
    if (use_port_1)
        kbdctl_send_cmd(CMD_ENABLE_PORT1);
    if (use_port_2)
        kbdctl_send_cmd(CMD_ENABLE_PORT2);

    // Register IRQs
    interrupts_register_irq(IRQ_PORT1, kbdctl_irq_port1);
    interrupts_register_irq(IRQ_PORT2, kbdctl_irq_port2);

    // Scan ports for devices and initialize drivers
    // In the process of enabling the interrupts, set the IRQ
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
    return result == RES_SELFTEST_OK;
}

// Flush the keyboard controller output buffer
static inline void kbdctl_flush_outbuf()
{
    while (kbdctl_outbuf_full())
        kbdctl_read_data();
}

// Read data from Keyboard controller
static inline uint8_t kbdctl_read_data()
{
    // Wait for data to be available
    while (!kbdctl_outbuf_full())
        pause();

    return inb(PORT_DATA);
}

// Read data from Keyboard controller with timeout (ms)
bool kbdctl_read_data_timeout(uint8_t *data, uint32_t timeout)
{
    uint32_t start = clock_get_system();

    // Wait for data to be available
    while (!kbdctl_outbuf_full())
    {
        pause();
        if (clock_get_system() - start > timeout)
            return false;
    }

    // Read data
    *data = inb(PORT_DATA);

    return true;
}

// Check if output buffer of the Keyboard controller is full
static inline bool kbdctl_outbuf_full()
{
    return kbdctl_read_sreg().outbuf_full;
}

// Check if input buffer of the Keyboard controller is full
static inline bool kbdctl_inbuf_full()
{
    return kbdctl_read_sreg().inbuf_full;
}

// Write data to PS2 port
static inline void kbdctl_write_data(uint8_t data, kbdctl_port port)
{
    // Send next byte to port 2
    if (port == PORT_2)
        kbdctl_send_cmd(CMD_WRITE_PORT2);

    // Wait until buffer is empty
    while (kbdctl_inbuf_full())
        pause();

    outb(PORT_DATA, data);
}

// Writes byte to a PS/2 port, handling resends
// Returns false in case of failure
static bool write_data_resend(uint8_t byte, kbdctl_port port)
{
    uint8_t data, retries = 5;
    do
    {
        // Limit number of retries
        if (retries == 0)
            return false;

        // Write byte
        kbdctl_write_data(byte, port);

        // Read first byte from device, that will be
        //  - 0xFE -> Resend
        //  - 0xFA -> ACK
        if (!kbdctl_read_data_timeout(&data, TIMEOUT))
            return false;

    } while (data == PS2_RESEND);

    return true;
}

// Reset device connected to a PS2 port, and read the results
// of it self-test sequence
// Returns true if the device is connected and its self-test sequence
// completed succesfully
static bool device_initialize(kbdctl_port port)
{
    klog("Initializing device: %d\n", port);

    // Send reset command
    if (!write_data_resend(PS2_RESET, port))
        return false;

    // Read reset result (2 bytes)
    uint8_t res;
    if (!kbdctl_read_data_timeout(&res, TIMEOUT))
        // No response from the device (device not present)
        goto fail;

    // Check if device self test was successful
    if (res != PS2_SELFTEST_OK)
    {
        klog("[KBDCTL] Port %d device self test failure: 0x%x\n", port, res);
        goto fail;
    }

    // Identify device
    device_type type = identify_device();

    klog("Device type: %d\n", type);

    return true;

fail:
    return false;
}

// Identifies device as part of its initialization process
static device_type identify_device()
{
    uint8_t data;
    if (!kbdctl_read_data_timeout(&data, TIMEOUT))
    {
        //  AT Keyboards don't send ID bytes
        return KEYBOARD;
    }

    // Mice send only one byte
    if (data == 0x00 || data == 0x03 || data == 0x04)
        return MOUSE;

    if (data == 0xAB)
    {
        if (!kbdctl_read_data_timeout(&data, TIMEOUT))
        {
            // Keyboads send two bytes, if no second byte is received
            // the device can't be identified
            return UNKNOWN;
        }

        return KEYBOARD;
    }

    return UNKNOWN;
}

// IRQ handler for PS2 port 1
static void kbdctl_irq_port1()
{
    klog("IRQ1\n");
    kbdctl_read_data();
}

// IRQ handler for PS2 port 2
static void kbdctl_irq_port2()
{
    klog("IRQ12\n");
    kbdctl_read_data();
}
