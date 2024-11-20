#include "proc/elf.h"

#include <string.h>

#include "config.h"
#include "mem/mem.h"
#include "mem/kalloc.h"
#include "mem/vmem.h"
#include "error.h"
#include "log.h"

// Configure debugging
#if DEBUG_ELF == 1
#define DEBUG
#endif

// ELF header
typedef struct __attribute__((packed))
{
    uint32_t magic;        // Magic number
    uint8_t class;         // 1 -> 32-bit, 2 -> 64-bit
    uint8_t data_enc;      // 1 -> Little endian, 2 -> Big endian
    uint8_t h_vers;        // ELF version
    uint8_t abi;           // OS ABI
    uint8_t _res[8];       // Unused
    uint16_t type;         // Type: 1 -> Relocatable, 2 -> Executable,
                           //       3 -> Shared, 4 -> Core
    uint16_t inst_set;     // Instruction set
    uint32_t vers;         // ELF version (duplicate ?)
    uint32_t entry;        // Program entrypoint
    uint32_t ph_offset;    // Program header table offset
    uint32_t sh_offset;    // Section header table offset
    uint32_t flags;        // Flags (unused)
    uint16_t header_size;  // Header size
    uint16_t ph_ent_size;  // Program header entry size
    uint16_t ph_ent_num;   // Program header table entry number
    uint16_t sh_ent_size;  // Section header entry size
    uint16_t sh_ent_num;   // Section header table entry number
    uint16_t st_entry_idx; // Indx of the string table in the section
                           // header table

} elf_header_t;

#define ELF_MAGIC 0x464C457F // 7C + "ELF" but reversed because Little Endian
#define ELF_CLASS_32BIT 1
#define ELF_CLASS_64BIT 2
#define ELF_ENC_LE 1
#define ELF_ENC_BE 2
#define ELF_ABI_SYSV 0
#define ELF_TYPE_RELOC 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_SHRD 3
#define ELF_TYPE_CORE 4
#define ELF_INSTSET_X86 0x03

// ELF program header
typedef struct
{
    uint32_t type;   // Type of segment
    uint32_t offset; // Offset into the file
    uint32_t vaddr;  // Virtual address
    uint32_t paddr;  // Physical address
    uint32_t filesz; // Number of bytes to copy
    uint32_t memsz;  // Number of bytes to allocate and clear
    uint32_t flags;  // Flags
    uint32_t align;  // Alignment
} elf_ph_ent_t;

#define ELF_PH_TYPE_NULL 0
#define ELF_PH_TYPE_LOAD 1
#define ELF_PH_TYPE_DYN 2
#define ELF_PH_TYPE_INTERP 3

// Internal function prototypes
int32_t check_elf_format(elf_header_t *header);
int32_t elf_vfs_read(vfs_file_handle_t file, uint8_t *buf,
                     uint32_t offset, uint32_t n, int64_t err);
int32_t do_load_program(vfs_file_handle_t file, elf_ph_ent_t *ph_table,
                        uint32_t ph_ent_n);
int32_t do_load_segment(vfs_file_handle_t file, elf_ph_ent_t *ph);

int32_t elf_load(vfs_file_handle_t file, void **entry)
{
    int32_t res;
    elf_ph_ent_t *ph_table = NULL;

    // Load ELF header into memory
    elf_header_t header;
    if ((res = elf_vfs_read(file, (uint8_t *)&header, 0, sizeof(elf_header_t),
                            E_NOTELF)) < 0)
        goto fail;

    kprintf("[ELF] Header read succesfully\n");

    // Check ELF format
    if ((res = check_elf_format(&header)) < 0)
        goto fail;

    kprintf("[ELF] Format OK\n");

    // Allocate space for program header table
    uint32_t ph_table_size = sizeof(elf_ph_ent_t) * header.ph_ent_num;
    if ((ph_table = kalloc(ph_table_size)) == NULL)
    {
        res = E_NOMEM;
        goto fail;
    }

    // Load program header table
    // It can be found at the file offset provided by the header
    if ((res = elf_vfs_read(file, (uint8_t *)ph_table, header.ph_offset,
                            ph_table_size, E_ELFFMT)) < 0)
        goto fail;

    // Load program
    if ((res = do_load_program(file, ph_table, header.ph_ent_num)) < 0)
        goto fail;

    // Free program header table
    kfree(ph_table);

    // Set entry point
    *entry = (void *)header.entry;

    return 0;

fail:
    kprintf("[ELF] Fail\n");
    // Free program header table
    if (ph_table)
        kfree(ph_table);
    return res;
}

/* Internal functions */

// Check the ELF header to verify that the header is actually
// a valid ELF file
// Returns 0 on success
int32_t check_elf_format(elf_header_t *header)
{
    // Check if file is actually an ELF
    if (header->magic != ELF_MAGIC)
        return E_NOTELF;

    // Validate parameters
    if (header->class != ELF_CLASS_32BIT ||
        header->data_enc != ELF_ENC_LE ||
        header->h_vers != 1 ||
        header->abi != ELF_ABI_SYSV ||
        header->type != ELF_TYPE_EXEC ||
        header->inst_set != ELF_INSTSET_X86 ||
        header->vers != 1)
        return E_ELFFMT;

    // Do some sanity checks
    if (header->ph_ent_size != sizeof(elf_ph_ent_t))
        return E_ELFFMT;

    return 0;
}

// Read an exact amount of bytes, otherwise return an arbitrary error passed
// in the err parameter
// It is a light wrapper over vfs_read()
// The err parameter makes it possible to specify the error that will be
int32_t elf_vfs_read(vfs_file_handle_t file, uint8_t *buf,
                     uint32_t offset, uint32_t n, int64_t err)
{
    int64_t res;
    if ((res = vfs_read(file, buf, offset, n)) < n)
    {
        if (res < 0)
            return res;

        // Not enough data
        return err;
    }

    return 0;
}

// Load program from ELF program segments
int32_t do_load_program(vfs_file_handle_t file, elf_ph_ent_t *ph_table,
                        uint32_t ph_ent_n)
{
    int64_t res;

    // Iterate over all segment
    for (uint32_t i = 0; i < ph_ent_n; i++)
    {
        // Ignore NULL segments
        if (ph_table[i].type == ELF_PH_TYPE_NULL)
            continue;

        // Fail on unsupported segment types
        if (ph_table[i].type != ELF_PH_TYPE_LOAD)
            return E_ELFFMT;

        // Load segment
        if ((res = do_load_segment(file, &ph_table[i])) < 0)
            return res;
    }

    return 0;
}

// Load ELF program segment
int32_t do_load_segment(vfs_file_handle_t file, elf_ph_ent_t *ph)
{
    int32_t res;

#ifdef DEBUG
    kprintf("[ELF] Loading segment: vaddr: 0x%x, memsz: %u, filesz: %u\n",
            ph->vaddr, ph->memsz, ph->filesz);
#endif

    // Compute memory information
    void *vaddr = (void *)ph->vaddr;
    void *page_start = vmem_page_aligned(vaddr);
    uint32_t n_pages = vmem_n_pages_pa(vaddr, ph->memsz);

    // Check that segment is not trying to load in the KVAS
    if (!vmem_validate_user_ptr(page_start, n_pages * MEM_PAGE_SIZE))
        return E_ELFFMT;

    // Allocate necessary memory
    if (!mem_make_avail(page_start, n_pages))
        return E_NOMEM;

    // Clear memory
    memset(page_start, 0, n_pages * MEM_PAGE_SIZE);

    // Load segment into memory
    if ((res = elf_vfs_read(file, vaddr, ph->offset, ph->filesz, E_ELFFMT)) < 0)
        return res;

    // Succesfully loaded segment!
    return 0;
}