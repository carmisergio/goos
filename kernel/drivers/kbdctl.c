#include "drivers/kbdctl.h"

#include <stddef.h>
#include "sys/io.h"

#include "log.h"
#include "panic.h"
#include "clock.h"
#include "cpu.h"
#include "int/interrupts.h"
#include "drivers/ps2.h"
#include "drivers/ps2kbd.h"

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
static inline void kbdctl_send_cmd(kbdctl_cmd cmd);
static inline kbdctl_sreg kbdctl_read_sreg();
static inline void kbdctl_write_cfg_byte(kbdctl_cfg_byte cfg_byte);
static inline kbdctl_cfg_byte kbdctl_read_cfg_byte();
static inline bool kbdctl_selftest();
static inline void kbdctl_flush_outbuf();
static inline bool kbdctl_outbuf_full();
static inline bool kbdctl_inbuf_full();
static inline uint8_t kbdctl_read_data();
static inline bool kbdctl_read_data_noblock(uint8_t *data);
bool kbdctl_read_data_timeout(uint8_t *data, uint32_t timeout);
static inline void write_data(uint8_t data);
static inline void write_data_port(uint8_t data, kbdctl_port port);
static bool write_data_resend(uint8_t byte, kbdctl_port port);
static bool device_initialize(device_type *type, kbdctl_port port);
static device_type identify_device();
static char *get_device_type_string(device_type type);
static void write_data_port_1(uint8_t data);
static void write_data_port_2(uint8_t data);
static void enable_port_2();
static void enable_port_1();
static void disable_port_1();
static void disable_port_2();
static void kbdctl_irq_port1();
static void kbdctl_irq_port2();

// Global driver objects
bool use_port_1, use_port_2;
ps2_callbacks port_1_driver, port_2_driver;

/* Public functions */

void kbdctl_init()
{
    kbdctl_cfg_byte cfg_byte;
    use_port_1 = true; // Port 1 is present on all controllers
    use_port_2 = false;
    device_type dev1_type, dev2_type;

    klog("[KBDCTL] Initializing controller...\n");

    // Disable devices
    kbdctl_send_cmd(CMD_DISABLE_PORT1);
    kbdctl_send_cmd(CMD_DISABLE_PORT2);

    // Flush output buffer
    kbdctl_flush_outbuf();

    // Initialize controller
    cfg_byte = kbdctl_read_cfg_byte();
    cfg_byte.port1_irq_en = 0;
    cfg_byte.port2_irq_en = 0;
    cfg_byte.port1_clock_dis = 1;
    cfg_byte.port2_clock_dis = 1;
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
    if (kbdctl_read_cfg_byte().port2_clock_dis == 0)
    {
        // Controller has two ports
        use_port_2 = true;

        // Disable second port again
        kbdctl_write_cfg_byte(cfg_byte);
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

    // Reset device 1
    if (use_port_1)
    {
        // Enable port 1
        cfg_byte.port1_clock_dis = 0;
        kbdctl_write_cfg_byte(cfg_byte);

        use_port_1 = device_initialize(&dev1_type, PORT_1);

        // We disable the port again to distinguish between messages coming
        // from the two ports during initialization
        cfg_byte.port1_clock_dis = 1;
        kbdctl_write_cfg_byte(cfg_byte);
    }

    // Reset device 2
    if (use_port_2)
    {
        // Enable port 2
        cfg_byte.port2_clock_dis = 0;
        kbdctl_write_cfg_byte(cfg_byte);

        use_port_2 = device_initialize(&dev2_type, PORT_2);

        // We disable the port again to distinguish between messages coming
        // from the two ports during initialization
        cfg_byte.port2_clock_dis = 1;
        kbdctl_write_cfg_byte(cfg_byte);
    }

    // Enable ports and start drivers
    if (use_port_1)
    {
        cfg_byte.port1_irq_en = 1;
        cfg_byte.port1_clock_dis = 0;
        kbdctl_write_cfg_byte(cfg_byte);
        interrupts_register_irq(IRQ_PORT1, kbdctl_irq_port1);

        // Start driver
        ps2_port port_1 = {
            .send_data = write_data_port_1,
            .enable = enable_port_1,
            .disable = disable_port_1,
        };

        bool init_result;
        switch (dev1_type)
        {
        case KEYBOARD:
            init_result = ps2kbd_init(&port_1_driver, port_1);
            break;
        default:
            init_result = false;
        }

        // If driver initialization failed, disable IRQ
        if (!init_result)
        {
            cfg_byte.port1_irq_en = 0;
            cfg_byte.port1_clock_dis = 1;
            kbdctl_write_cfg_byte(cfg_byte);
            interrupts_unregister_irq(IRQ_PORT1, kbdctl_irq_port1);
        }
    }

    if (use_port_2)
    {
        cfg_byte.port2_irq_en = 1;
        cfg_byte.port2_clock_dis = 1;
        kbdctl_write_cfg_byte(cfg_byte);
        kbdctl_send_cmd(CMD_ENABLE_PORT2);
        interrupts_register_irq(IRQ_PORT2, kbdctl_irq_port2);

        // Start driver
        ps2_port port_2 = {
            .send_data = write_data_port_2,
            .enable = enable_port_2,
            .disable = disable_port_2,
        };

        bool init_result;
        switch (dev2_type)
        {
        case KEYBOARD:
            init_result = ps2kbd_init(&port_2_driver, port_2);
            break;
        default:
            init_result = false;
        }

        // If driver initialization failed, disable IRQ
        if (!init_result)
        {
            cfg_byte.port2_irq_en = 0;
            cfg_byte.port2_clock_dis = 1;
            kbdctl_write_cfg_byte(cfg_byte);
            interrupts_unregister_irq(IRQ_PORT2, kbdctl_irq_port2);
        }
    }
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
    kbdctl_send_cmd(CMD_WRITE_BYTE_0);
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

// Read data from Keyboard controller,
// exit immediately if no data is present
// Returns false if no data was available
static inline bool kbdctl_read_data_noblock(uint8_t *data)
{
    // Wait for data to be available
    if (!kbdctl_outbuf_full())
        return false;

    *data = inb(PORT_DATA);

    return true;
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

// Write data to the keyboard controller outupt buffer
static inline void write_data(uint8_t data)
{
    // Wait until buffer is empty
    while (kbdctl_inbuf_full())
        pause();

    outb(PORT_DATA, data);
}

// Write data to PS2 port
static inline void write_data_port(uint8_t data, kbdctl_port port)
{
    // Send next byte to port 2
    if (port == PORT_2)
        kbdctl_send_cmd(CMD_WRITE_PORT2);

    write_data(data);
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
        write_data_port(byte, port);

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
static bool device_initialize(device_type *type, kbdctl_port port)
{
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
    *type = identify_device();
    if (*type == UNKNOWN)
        return false;

    klog("[KBDCTL] Detected %s on port %d\n",
         get_device_type_string(*type), port);

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

static char *get_device_type_string(device_type type)
{
    switch (type)
    {
    case KEYBOARD:
        return "Keyboard";
    case MOUSE:
        return "Mouse";
    default:
        return NULL;
    }
}

static void write_data_port_1(uint8_t data)
{
    write_data_port(data, PORT_1);
}

static void write_data_port_2(uint8_t data)
{
    write_data_port(data, PORT_2);
}

static void enable_port_1()
{
    kbdctl_send_cmd(CMD_ENABLE_PORT1);
}

static void enable_port_2()
{
    kbdctl_send_cmd(CMD_ENABLE_PORT2);
}

static void disable_port_1()
{
    kbdctl_send_cmd(CMD_DISABLE_PORT1);
}

static void disable_port_2()
{
    kbdctl_send_cmd(CMD_DISABLE_PORT2);
}

// IRQ handler for PS2 port 1
static void kbdctl_irq_port1()
{
    uint8_t data;
    if (kbdctl_read_data_noblock(&data))
        port_1_driver.got_data_callback(data);
}

// IRQ handler for PS2 port 2
static void kbdctl_irq_port2()
{
    uint8_t data;
    if (kbdctl_read_data_noblock(&data))
        port_2_driver.got_data_callback(data);
}
