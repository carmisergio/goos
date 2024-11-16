#include "drivers/kbdctl.h"

#include <stddef.h>
#include "sys/io.h"

#include "config.h"
#include "log.h"
#include "panic.h"
#include "clock.h"
#include "cpu.h"
#include "int/interrupts.h"
#include "drivers/ps2.h"
#include "drivers/ps2kbd/ps2kbd.h"

// Configure debugging
#if DEBUG_KBDCTL == 1
#define DEBUG
#endif

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

#define TIMEOUT 100       // (ms)
#define POST_TIMEOUT 1000 // (ms)
#define RESEND_RETRIES 10

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
static inline void write_cmd(kbdctl_cmd cmd);
static inline kbdctl_sreg read_sreg();
static inline void write_cfg_byte(kbdctl_cfg_byte cfg_byte);
static inline kbdctl_cfg_byte read_cfg_byte();
static inline bool kbdctl_selftest();
static inline void flush_outbuf();
static inline bool outbuf_full();
static inline bool inbuf_full();
static inline uint8_t read_data();
static inline bool read_data_noblock(uint8_t *data);
bool read_data_timeout(uint8_t *data, uint32_t timeout);
static inline void write_data(uint8_t data);
static inline void write_data_port(uint8_t data, kbdctl_port port);
static bool device_initialize(device_type *type, kbdctl_port port);
static device_type device_identify();
static char *get_device_type_string(device_type type);
static bool device_self_test(kbdctl_port port);
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

    kprintf("[KBDCTL] Initializing controller...\n");

    // Disable devices
    write_cmd(CMD_DISABLE_PORT1);
    write_cmd(CMD_DISABLE_PORT2);

    // Flush output buffer
    flush_outbuf();

    // Initialize controller
    cfg_byte = read_cfg_byte();
    cfg_byte.port1_irq_en = 0;
    cfg_byte.port2_irq_en = 0;
    cfg_byte.port1_clock_dis = 1;
    cfg_byte.port2_clock_dis = 1;
    cfg_byte.port1_trans_en = 0;
    cfg_byte.zero = 0;
    write_cfg_byte(cfg_byte);

#ifdef DEBUG
    kprintf("[KBDCTL] Config byte (0): 0x%x\n", read_cfg_byte().bits);
#endif

    // Controller self test
    if (!kbdctl_selftest())
    {
        kprintf("[KBDCTL] Selftest failure!\n");

        // Exit from driver
        return;
    }

    // Self test may reset controller configuration,
    // make sure its set correctly
    write_cfg_byte(cfg_byte);

#ifdef DEBUG
    kprintf("[KBDCTL] Config byte (1): 0x%x\n", read_cfg_byte().bits);
#endif

    // Check if the controller has two ports
    write_cmd(CMD_ENABLE_PORT2);
    if (read_cfg_byte().port2_clock_dis == 0)
    {
#ifdef DEBUG
        kprintf("[KBDCTL] Second port detected\n");
#endif
        // Controller has two ports
        use_port_2 = true;

        // Disable second port again
        write_cfg_byte(cfg_byte);
    }

#ifdef DEBUG
    kprintf("[KBDCTL] Config byte (2): 0x%x\n", read_cfg_byte().bits);
#endif

    // Perform interface checks
    write_cmd(CMD_TEST_PORT1);
    if (read_data() != RES_INT_TEST_OK)
    {
        kprintf("[KBDCTL] Port 1 interface test failed!\n");
        // Disable port 1
        use_port_1 = false;
    }
    if (use_port_2)
    {
        write_cmd(CMD_TEST_PORT2);
        if (read_data() != RES_INT_TEST_OK)
        {
            kprintf("[KBDCTL] Port 2 interface test failed!\n");
            // Disable port 2
            use_port_2 = false;
        }
    }

#ifdef DEBUG
    kprintf("[KBDCTL] Config byte (3): 0x%x\n", read_cfg_byte().bits);
#endif

    // Reset and identify device 1
    if (use_port_1)
    {
        // Enable port 1
        cfg_byte.port1_clock_dis = 0;
        write_cfg_byte(cfg_byte);
        write_cmd(CMD_ENABLE_PORT1);

        use_port_1 = device_initialize(&dev1_type, PORT_1);

        // We disable the port again to distinguish between messages coming
        // from the two ports during initialization
        cfg_byte.port1_clock_dis = 1;
        write_cfg_byte(cfg_byte);
        write_cmd(CMD_DISABLE_PORT1);
    }

    // Reset and identify device 2
    if (use_port_2)
    {
        // Enable port 2
        cfg_byte.port2_clock_dis = 0;
        write_cfg_byte(cfg_byte);
        write_cmd(CMD_ENABLE_PORT2);

        use_port_2 = device_initialize(&dev2_type, PORT_2);

        // We disable the port again to distinguish between messages coming
        // from the two ports during initialization
        cfg_byte.port2_clock_dis = 1;
        write_cfg_byte(cfg_byte);
        write_cmd(CMD_DISABLE_PORT2);
    }

#ifdef DEBUG
    kprintf("[KBDCTL] Config byte (4): 0x%x\n", read_cfg_byte().bits);
#endif

    // Enable port interrupts
    cfg_byte.port1_irq_en = 1;
    cfg_byte.port2_irq_en = 1;
    write_cfg_byte(cfg_byte);

    // Register interrupts and start drivers
    if (use_port_1)
    {
        // #ifdef DEBUG
        //         kprintf("[KBDCTL] Config byte (5): 0x%x\n", kbdctl_read_cfg_byte().bits);
        // #endif

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
#ifdef DEBUG
            kprintf("[KBDCTL] Port 1 driver initialization failed!\n");
#endif
            write_cmd(CMD_DISABLE_PORT1);
            interrupts_unregister_irq(IRQ_PORT1, kbdctl_irq_port1);
        }
    }

    if (use_port_2)
    {
        // #ifdef DEBUG
        //         kprintf("[KBDCTL] Config byte (6): 0x%x\n", kbdctl_read_cfg_byte().bits);
        // #endif

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
#ifdef DEBUG
            kprintf("[KBDCTL] Port 2 driver initialization failed!\n");
#endif
            write_cmd(CMD_DISABLE_PORT2);
            interrupts_unregister_irq(IRQ_PORT2, kbdctl_irq_port2);
        }
    }
}

void kbdctl_reset_cpu()
{
    write_cmd(CMD_PULSE_OUTP0);
}

/* Internal functions */

// Send command to keyboard controller
static inline void write_cmd(kbdctl_cmd cmd)
{
    // Wait for buffer to be empty
    while (inbuf_full())
        pause();

    outb(PORT_CMD, cmd);
}

// Read keyboard controller status register
static inline kbdctl_sreg read_sreg()
{
    kbdctl_sreg sreg;
    sreg.bits = inb(PORT_CMD);
    return sreg;
}

// Write data to the keyboard controller outupt buffer
static inline void write_data(uint8_t data)
{
    // Wait until buffer is empty
    while (inbuf_full())
        pause();

    outb(PORT_DATA, data);
}

// Read data from Keyboard controller output buffer
static inline uint8_t read_data()
{
    // Wait for data to be available
    while (!outbuf_full())
        pause();

    return inb(PORT_DATA);
}

// Read data from Keyboard controller output buffer,
// exit immediately if no data is present
// Returns false if no data was available
static inline bool read_data_noblock(uint8_t *data)
{
    // Wait for data to be available
    if (!outbuf_full())
        return false;

    *data = inb(PORT_DATA);

    return true;
}

// Read data from Keyboard controller output buffer with timeout (ms)
bool read_data_timeout(uint8_t *data, uint32_t timeout)
{
    uint32_t start = clock_get_system();

    // Wait for data to be available
    while (!outbuf_full())
    {
        pause();
        if (clock_get_system() - start > timeout)
            return false;
    }

    // Read data
    *data = inb(PORT_DATA);

    return true;
}

// Check if the output buffer (from device side) is full
static inline bool outbuf_full()
{
    return read_sreg().outbuf_full == 1;
}

// Check if the input buffer (from device side) is full
static inline bool inbuf_full()
{
    return read_sreg().inbuf_full == 1;
}

// Write keyboard controller configuration byte
static inline void write_cfg_byte(kbdctl_cfg_byte cfg_byte)
{
    write_cmd(CMD_WRITE_BYTE_0);
    write_data(cfg_byte.bits);
}

// Read keyboard controller configuration byte
static inline kbdctl_cfg_byte read_cfg_byte()
{
    kbdctl_cfg_byte cfg_byte;
    write_cmd(CMD_READ_BYTE_0);
    cfg_byte.bits = read_data();
    return cfg_byte;
}

// Perform keyboard controller self test
static inline bool kbdctl_selftest()
{
    // Start self test
    write_cmd(CMD_TEST_KBDCTL);

    // Read result
    uint8_t result = read_data();
    return result == RES_SELFTEST_OK;
}

// Flush the keyboard controller output buffer
static inline void flush_outbuf()
{
    while (outbuf_full())
        inb(PORT_DATA);
}

// Write data to PS2 port
static inline void write_data_port(uint8_t data, kbdctl_port port)
{
    // Send next byte to port 2
    if (port == PORT_2)
        write_cmd(CMD_WRITE_PORT2);

    write_data(data);

#ifdef DEBUG
    kprintf("[KBDCTL] writing to port %d : data = 0x%x\n", port, data);
#endif
}

// Reset device connected to a PS2 port, and read the results
// of it self-test sequence
// Returns true if the device is connected and its self-test sequence
// completed succesfully
static bool device_initialize(device_type *type, kbdctl_port port)
{
#ifdef DEBUG
    kprintf("[KBDCTL] Resetting device %d\n", port);
#endif

    if (!device_self_test(port))
        return false;

    // Identify device
    *type = device_identify();
    if (*type == UNKNOWN)
        return false;

    kprintf("[KBDCTL] Detected %s on port %d\n",
            get_device_type_string(*type), port);

    return true;
}

// Perform PS/2 device self test
// Returns false in case of failure
static bool device_self_test(kbdctl_port port)
{
    uint8_t data1, data2, retries = RESEND_RETRIES;

    // Send reset command, handling resends
    do
    {
        // Limit number of resends
        if (retries == 0)
        {
#ifdef DEBUG
            kprintf("[KBDCTL] Port %d device self test failure: retries exceeded\n", port);
#endif
            return false;
        }

        // Write reset byte
        write_data_port(PS2_RESET, port);

        // Read first byte from device, that will be
        //  - 0xFE -> Resend
        //  - 0xFA -> ACK
        //  - 0xAA -> Self-test OK
        // 0xFA and 0xAA will be sent in different order based on the
        // controller. Because why the hell not.
        if (!read_data_timeout(&data1, POST_TIMEOUT))
        {
#ifdef DEBUG
            kprintf("[KBDCTL] Port %d device self test failure: no response\n", port);
#endif
            return false;
        }
        retries--;

    } while (data1 == PS2_RESEND);

    // Consume other byte of the 0xFA, 0xAA sequence
    if (!read_data_timeout(&data2, POST_TIMEOUT))
    {
#ifdef DEBUG
        kprintf("[KBDCTL] Port %d device self test failure: no response\n", port);
#endif
        return false;
    }

    // Handle different orders
    if ((!(data1 == PS2_ACK && data2 == PS2_SELFTEST_OK) &&
         !(data1 == PS2_SELFTEST_OK && data2 == PS2_ACK)))
    {
        kprintf("[KBDCTL] Port %d device self test failure: 0x%x%x\n", port, data1, data2);
        return false;
    }

    return true;
}

// Identifies device as part of its initialization process
static device_type device_identify()
{
    uint8_t data;
    if (!read_data_timeout(&data, TIMEOUT))
    {
        //  AT Keyboards don't send ID bytes
        return KEYBOARD;
    }

#ifdef DEBUG
    kprintf("[KBDCTL] ID byte 0: 0x%x\n", data);
#endif

    // Mice send only one byte
    if (data == 0x00 || data == 0x03 || data == 0x04)
        return MOUSE;

    //     if (data == 0xAB || data == 0xF0 || data == 0xF2)
    //     {
    //         // Read second byte that some keyboards send
    //         if (read_data_timeout(&data, TIMEOUT))
    //         {

    // #ifdef DEBUG
    //             kprintf("[KBDCTL] ID byte 1: 0x%x\n", data);
    // #endif
    //         };

    //         return KEYBOARD;
    //     }

    if (read_data_timeout(&data, TIMEOUT))
    {
#ifdef DEBUG
        kprintf("[KBDCTL] ID byte 1: 0x%x\n", data);
#endif
    };

    return KEYBOARD;
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
    // kbdctl_cfg_byte cfg_byte = kbdctl_read_cfg_byte();
    // cfg_byte.port1_clock_dis = 0;
    // cfg_byte.port1_irq_en = 1;
    // kbdctl_write_cfg_byte(cfg_byte);
    write_cmd(CMD_ENABLE_PORT1);
}

static void enable_port_2()
{
    // kbdctl_cfg_byte cfg_byte = kbdctl_read_cfg_byte();
    // cfg_byte.port2_clock_dis = 0;
    // cfg_byte.port2_irq_en = 1;
    // kbdctl_write_cfg_byte(cfg_byte);
    write_cmd(CMD_ENABLE_PORT2);
}

static void disable_port_1()
{
    // kbdctl_cfg_byte cfg_byte = kbdctl_read_cfg_byte();
    // cfg_byte.port1_clock_dis = 1;
    // cfg_byte.port1_irq_en = 0;
    // kbdctl_write_cfg_byte(cfg_byte);
    write_cmd(CMD_DISABLE_PORT1);
}

static void disable_port_2()
{
    // kbdctl_cfg_byte cfg_byte = kbdctl_read_cfg_byte();
    // cfg_byte.port2_clock_dis = 1;
    // cfg_byte.port2_irq_en = 0;
    // kbdctl_write_cfg_byte(cfg_byte);
    write_cmd(CMD_DISABLE_PORT2);
}

// IRQ handler for PS2 port 1
static void kbdctl_irq_port1()
{
#ifdef DEBUG
    kprintf("[KBDCTL] Port 1 IRQ: ");
#endif
    uint8_t data = 0;
    if (read_data_noblock(&data))
    {
#ifdef DEBUG
        kprintf("data = 0x%x\n", data);
#endif
        port_1_driver.got_data_callback(data);
    }
#ifdef DEBUG
    else
    {
        kprintf("no data\n", data);
    }
#endif
}

// IRQ handler for PS2 port 2
static void kbdctl_irq_port2()
{
#ifdef DEBUG
    kprintf("[KBDCTL] Port 2 IRQ: ");
#endif
    uint8_t data = 0;
    if (read_data_noblock(&data))
    {
#ifdef DEBUG
        kprintf("data = 0x%x\n", data);
#endif
        port_2_driver.got_data_callback(data);
    }
#ifdef DEBUG
    else
    {
        kprintf("no data\n", data);
    }
#endif
}
