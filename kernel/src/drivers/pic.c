#include "drivers/pic.h"

#include "sys/io.h"

#include "log.h"

// NOTE: see http://www.brokenthorn.com/Resources/OSDevPic.html
//       for documentaiton on the PIC

// Address definions
#define PIC1_BASE 0x20 // Master PIC
#define PIC2_BASE 0xA0 // Slave PIC
#define PIC1_COMMAND PIC1_BASE
#define PIC1_DATA (PIC1_BASE + 1)
#define PIC2_COMMAND PIC2_BASE
#define PIC2_DATA (PIC2_BASE + 1)

// Commands
#define PIC_CMD_EOI 0x20

// PIC flags
#define ICW1 0x10
#define ICW1_IC4 (1 << 0)  /* Indicates that ICW4 will be present */
#define ICW1_SNGL (1 << 1) /* Single (cascade) mode */
#define ICW1_ADI4 (1 << 2) /* Call address interval 4 (8) */
#define ICW1_LTIM (1 << 3) /* Level triggered (edge) mode */
#define ICW3_MASTER_IRQ0 (1 << 0)
#define ICW3_MASTER_IRQ1 (1 << 1)
#define ICW3_MASTER_IRQ2 (1 << 2)
#define ICW3_MASTER_IRQ3 (1 << 3)
#define ICW3_MASTER_IRQ4 (1 << 4)
#define ICW3_MASTER_IRQ5 (1 << 5)
#define ICW3_MASTER_IRQ6 (1 << 6)
#define ICW3_MASTER_IRQ7 (1 << 7)
#define ICW3_SLAVE_IRQ0 0
#define ICW3_SLAVE_IRQ1 1
#define ICW3_SLAVE_IRQ2 2
#define ICW3_SLAVE_IRQ3 3
#define ICW3_SLAVE_IRQ4 4
#define ICW3_SLAVE_IRQ5 5
#define ICW3_SLAVE_IRQ6 6
#define ICW3_SLAVE_IRQ7 7
#define ICW4_8086 (1 << 0)   /* 8086/88 (MCS-80/85) mode */
#define ICW4_AEOI (1 << 1)   /* Auto (normal) EOI */
#define ICW4_MASTER (1 << 2) // In buffered mode, select master
#define ICW4_BUF (1 << 3)
#define ICW4_SFNM (1 << 4)
#define OCW3 0x8
#define OCW3_READ_IRR 0b10
#define OCW3_READ_ISR 0b11

// Function prototypes
static inline void pic_int_send_eoi(uint16_t base);
static inline bool pic_int_is_spuious(uint16_t base);

void pic_init(uint8_t start_vec)
{
    // ICW1
    outb(PIC1_COMMAND, ICW1 | ICW1_IC4); // starts the initialization sequence (in cascade mode)
    outb(PIC2_COMMAND, ICW1 | ICW1_IC4);

    // ICW2
    outb(PIC1_DATA, start_vec);     // ICW2: Master PIC vector offset
    outb(PIC2_DATA, start_vec + 8); // ICW2: Slave PIC vector offset

    // ICW3
    outb(PIC1_DATA, ICW3_MASTER_IRQ2); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC2_DATA, ICW3_SLAVE_IRQ2);  // ICW3: tell Slave PIC its cascade identity (0000 0010)

    // ICW4
    outb(PIC1_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    outb(PIC2_DATA, ICW4_8086);

    // Initialization done, null data registers
    outb(PIC1_DATA, 0);
    outb(PIC2_DATA, 0);
}

void pic_send_eoi(uint8_t irq)
{
    // If the interrupt belongs to the slave PIC,
    // and if so send EOI to it
    if (irq >= 8)
        pic_int_send_eoi(PIC2_BASE);

    // Send EOI to master PIC
    pic_int_send_eoi(PIC1_BASE);
}

bool pic_check_spurious(uint8_t irq)
{
    if (irq <= 7)
    {
        // Master PIC spurious interrupt
        if (pic_int_is_spuious(PIC1_BASE))
        {
            kprintf("[PIC] Master spurious interrupt!\n");
            return true;
        }
    }
    else
    {
        // Slave PIC spurious interrupt
        if (pic_int_is_spuious(PIC2_BASE))
        {
            // Send EOI to master PIC
            pic_int_send_eoi(PIC1_BASE);
            kprintf("[PIC] Slave spurious interrupt!\n");
            return true;
        }
    }
    return false;
}

// Send End of Interrupt command to PIC
static inline void pic_int_send_eoi(uint16_t base)
{
    outb(base, PIC_CMD_EOI);
}

// Read value of the In Service Register of PIC
static inline uint8_t pic_read_isr(uint16_t base)
{
    // Ask PICto read its ISR register
    outb(base, OCW3 | OCW3_READ_ISR);

    // Read register value
    return inb(base);
}

// Check if the current IRQ is spurious
static inline bool pic_int_is_spuious(uint16_t base)
{
    // Read ISR value
    return pic_read_isr(base) == 0;
}