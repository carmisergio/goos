#include "mem/kalloc.h"

#include "stdbool.h"

#include "mem/mem.h"
#include "mem/const.h"
#include "mem/vmem.h"
#include "log.h"
#include "panic.h"

#define DEBUG

// Number of memory pages allocted initially
#define INITIAL_PAGES 2
#define MIN_ALLOC 4 // Minimum allocation (bytes)

// Memory block header
typedef struct mem_block block_t;
struct mem_block
{
    // Size of block in bytes
    size_t size;

    // Block chain pointers
    block_t *sizelst_next;
    block_t *sizelst_prev;
    block_t *addrlst_next;
    block_t *addrlst_prev;
};

// Internal function prototypes
static block_t *allocate_new_pages(size_t n);
static block_t *block_chain_insert(block_t *new);
static void block_chain_remove(block_t *bptr);
static block_t *defrag_block(block_t *bptr);
static block_t *sizelst_find_insert_pos(block_t *new);
static block_t *addrlst_find_insert_pos(block_t *new);
static block_t *sizelst_find_insert_pos_start(block_t *new, block_t *start);
static void sizelst_insert_after(block_t *after, block_t *new);
static void addrlst_insert_after(block_t *after, block_t *new);
static void sizelst_remove(block_t *bptr);
static void addrlst_remove(block_t *bptr);
static inline void *bptr_to_mptr(block_t *bptr);
static inline block_t *mptr_to_bptr(void *bptr);
static block_t *get_block(size_t n);

// Global objects
block_t *sizelst_head;
block_t *addrlst_head;

void kalloc_init()
{
#ifdef DEBUG
    kdbg("[KALLOC] Initializing...\n");
#endif

    // Inizialize lists
    sizelst_head = NULL;
    addrlst_head = NULL;

    // Allocate one page of memory for future use
    block_t *bptr;
    if ((bptr = allocate_new_pages(INITIAL_PAGES)) == NULL)
        panic("KALLOC_INIT_NOMEM",
              "Unable to initialize initial memory during kalloc initialization");

    // Add it to the block chain
    block_chain_insert(bptr);
}

void *kalloc(size_t n)
{
    // Clamp size to minimum allocation size
    if (n < MIN_ALLOC)
        n = MIN_ALLOC;

    // Find suitable block in size list
    block_t *bptr = get_block(n);

    // If no suitable block was found, we need to allocate
    if (bptr == NULL)
    {
        size_t n_pages = vmem_n_pages(n);

        // If allocation fails, return NULL
        if ((bptr = allocate_new_pages(n_pages)) == NULL)
            return NULL;

        // Defragment block
        block_t *addrlst_pos = addrlst_find_insert_pos(bptr);
        addrlst_insert_after(addrlst_pos, bptr);
        bptr = defrag_block(bptr);

        addrlst_remove(bptr);
    }

    // Check if we need to partition the block
    // (We partition the block only if there's space for another block
    // header and minimum allocation size)
    if (bptr->size > n + sizeof(block_t) + MIN_ALLOC)
    {
        // Create new block
        block_t *new_bptr = (block_t *)((char *)bptr + sizeof(block_t) + n);
        new_bptr->size = bptr->size - n - sizeof(block_t);

        // Shrink current block
        bptr->size = n;

        //// Insert new block in block chain

        // Add to address list
        // We already know the position because it necessarely has to be after
        // the one before the current block
        addrlst_insert_after(bptr->addrlst_prev, new_bptr);

        // Defragment
        new_bptr = defrag_block(new_bptr);

        // Add to size list
        block_t *sizelst_pos = sizelst_find_insert_pos(new_bptr);
        sizelst_insert_after(sizelst_pos, new_bptr);
    }

    return bptr_to_mptr(bptr);
}

void kfree(void *ptr)
{
    // Add block to block chain
    block_chain_insert(mptr_to_bptr(ptr));
}

// Print memory chains
void kalloc_dbg_block_chain()
{
    block_t *cur;

    kprintf("### Memory block chain: \n");
    kprintf("Size list: \n");
    cur = sizelst_head;
    while (cur != NULL)
    {
        kprintf(" [0x%x, size = %d]\n", cur, cur->size);
        cur = cur->sizelst_next;
    }
    kprintf("Address list: \n");
    cur = addrlst_head;
    while (cur != NULL)
    {
        kprintf(" [0x%x, size = %d]\n", cur, cur->size);
        cur = cur->addrlst_next;
    }
}

/* Internal functions */

// Allocate new pages and construct the block header for them
// Returns a pointer to the new formed block, NULL
// if allocation was unsuccesful
static block_t *allocate_new_pages(size_t n)
{
#ifdef DEBUG
    kdbg("[KALLOC] Allocating new pages: %d\n", n);
#endif

    // Get pages from virtual memory manager
    void *mem = mem_palloc_k(n);
    if (mem == MEM_FAIL)
        return NULL;

    // Set up block header
    block_t *bptr = (block_t *)mem;

    // Usable memory size is total size - size of header
    bptr->size = (n * MEM_PAGE_SIZE) - sizeof(block_t);

    return bptr;
}

// Insert block into the block chains in the correct position
// Returns a pointer to the defragmented block
static block_t *block_chain_insert(block_t *bptr)
{

    // Insert into address list
    block_t *addrlst_pos = addrlst_find_insert_pos(bptr);
    addrlst_insert_after(addrlst_pos, bptr);

    // Defragment block
    bptr = defrag_block(bptr);

    // Insert into size list
    block_t *sizelst_pos = sizelst_find_insert_pos(bptr);
    sizelst_insert_after(sizelst_pos, bptr);

    return bptr;
}

// Remove block from the block chain
static void block_chain_remove(block_t *bptr)
{
    // Remove from size list
    sizelst_remove(bptr);

    // Remove from address list
    addrlst_remove(bptr);
}

// Join block with contiguous blocks, if possible
// Returns a pointer to the defragmented block
// NOTE: operates only on the address list and doesn't update the size list
static block_t *defrag_block(block_t *bptr)
{
    bool has_defragged = false;

    // Defragment left
    if (bptr->addrlst_prev != NULL)
    {
        // Check if blocks are contiguous
        if ((block_t *)((char *)bptr->addrlst_prev +
                        bptr->addrlst_prev->size + sizeof(block_t)) == bptr)
        {
            has_defragged = true;

            // Remove current block from block chain
            addrlst_remove(bptr);

            // Extend previous block
            bptr->addrlst_prev->size += sizeof(block_t) + bptr->size;

            // Make bptr the joined block, so the right check works
            bptr = bptr->addrlst_prev;

            // Remove joint block from size list
            sizelst_remove(bptr);
        }
    }

    // Defragment right
    if (bptr->addrlst_next != NULL)
    {
        // Check if blocks are contiguous
        if ((block_t *)((char *)bptr + sizeof(block_t) + bptr->size) ==
            bptr->addrlst_next)
        {
            has_defragged = true;

            // Extend current block
            bptr->size += sizeof(block_t) + bptr->addrlst_next->size;

            // Remove next block from block chain
            sizelst_remove(bptr->addrlst_next);
            addrlst_remove(bptr->addrlst_next);
        }
    }

    // // If it has defragged, reposition block in the size list
    // if (has_defragged)
    // {
    //     // Temporarely remove block from size list
    //     sizelst_remove(bptr);

    //     // Find new position
    //     // New size can only be bigger than the previous, so start looking for a new position
    //     // from the current location
    //     block_t *pos = sizelst_find_insert_pos_start(bptr, bptr->addrlst_prev);

    //     // Put block back in size list
    //     sizelst_insert_after(pos, bptr);
    // }

    return bptr;
}

// Find position in size list where to insert new node
// Retruns a pointer to the node AFTER which to insert the new node,
// NULL if the correct position is the head of the list
static block_t *sizelst_find_insert_pos(block_t *new)
{
    block_t *cur = sizelst_head; // Start searching from the head
    block_t *after = NULL;

    // Traverse size list and find either end of list or first node
    // with bigger size, in which case the correct position is after the
    // previous node
    while (cur != NULL && cur->size < new->size)
    {
        after = cur;
        cur = cur->sizelst_next;
    }

    return after;
}

// Find position in adress list where to insert new node
// Retruns a pointer to the node AFTER which to insert the new node,
// NULL if the correct position is the head of the list
static block_t *addrlst_find_insert_pos(block_t *new)
{
    block_t *cur = addrlst_head; // Start searching from the head
    block_t *after = NULL;

    // Traverse size list and find either end of list or first node
    // with higher addresss, in which case the correct position is after the
    // previous node
    while (cur != NULL && cur < new)
    {
        after = cur;
        cur = cur->addrlst_next;
    }

    return after;
}

// Find position in size list where to insert new node, with starting point
// Starts searching from head if starting point is NULL
// Retruns a pointer to the node AFTER which to insert the new node,
// NULL if the correct position is the head of the list
static block_t *sizelst_find_insert_pos_start(block_t *new, block_t *start)
{
    block_t *cur = start == NULL ? sizelst_head : start;
    block_t *after = NULL;

    // Traverse size list and find either end of list or first node
    // with bigger size, in which case the correct position is after the
    // previous node
    while (cur != NULL && cur->size < new->size)
    {
        after = cur;
        cur = cur->sizelst_next;
    }

    return after;
}

// Insert in size list after node
static void sizelst_insert_after(block_t *after, block_t *new)
{

    new->sizelst_prev = after;

    if (after == NULL)
    {
        // If pointer is NULL, insert at head

        // Next of new node is the current head
        new->sizelst_next = sizelst_head;

        // If node at head, update it
        if (sizelst_head != NULL)
            sizelst_head->sizelst_prev = new;

        // Set new head
        sizelst_head = new;
    }
    else
    {
        // Next of new node is the next of the node we're inserting after
        new->sizelst_next = after->sizelst_next;

        // If there is a node after the one we're inserting, update its previous pointer
        if (after->sizelst_next != NULL)
            after->sizelst_next->sizelst_prev = new;

        // Update previous node
        after->sizelst_next = new;
    }
}

// Insert in address list after node
static void addrlst_insert_after(block_t *after, block_t *new)
{

    new->addrlst_prev = after;

    if (after == NULL)
    {
        // If pointer is NULL, insert at head

        // Next of new node is the current head
        new->addrlst_next = addrlst_head;

        // If node at head, update it
        if (addrlst_head != NULL)
            addrlst_head->addrlst_prev = new;

        // Set new head
        addrlst_head = new;
    }
    else
    {
        // Next of new node is the next of the node we're inserting after
        new->addrlst_next = after->addrlst_next;

        // If there is a node after the one we're inserting, update its previous pointer
        if (after->addrlst_next != NULL)
            after->addrlst_next->addrlst_prev = new;

        // Update previous node
        after->addrlst_next = new;
    }
}

// Remove node from size list
static void sizelst_remove(block_t *bptr)
{
    // Update left side
    if (bptr->sizelst_prev != NULL)
        // Update previous node
        bptr->sizelst_prev->sizelst_next = bptr->sizelst_next;
    else
        // Update head
        sizelst_head = bptr->sizelst_next;

    // Update right side
    if (bptr->sizelst_next != NULL)
        bptr->sizelst_next->sizelst_prev = bptr->sizelst_prev;
}

// Remove node from address list
static void addrlst_remove(block_t *bptr)
{
    // Update left side
    if (bptr->addrlst_prev != NULL)
        // Update previous node
        bptr->addrlst_prev->addrlst_next = bptr->addrlst_next;
    else
        // Update head
        addrlst_head = bptr->addrlst_next;

    // Update right side
    if (bptr->addrlst_next != NULL)
        bptr->addrlst_next->addrlst_prev = bptr->addrlst_prev;
}

// Get memory address for a block
static inline void *bptr_to_mptr(block_t *bptr)
{
    // Memory starts after block header
    return (char *)bptr + sizeof(block_t);
}

// Get block pointer from the memory address
static inline block_t *mptr_to_bptr(void *bptr)
{
    // Memory starts after block header
    return (block_t *)((char *)bptr - sizeof(block_t));
}

// Find block of at least given size
// #### Parameters:
//   - n: size
// #### Returns:
//   NULL if no suitable block found
static block_t *get_block(size_t n)
{
    block_t *cur = sizelst_head;

    while (cur != NULL)
    {
        if (cur->size >= n)
        {
            // Remove block from the block chain
            block_chain_remove(cur);
            return cur;
        }

        cur = cur->sizelst_next;
    }

    return NULL;
}
