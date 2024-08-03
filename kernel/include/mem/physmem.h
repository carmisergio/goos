#ifndef _PHYSMEM_H
#define _PHYSMEM_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#define PHYSMEM_NULL (void *)UINT32_MAX

    /*
     * Initialize physical memory page allocator
     *
     * Params:
     *     multiboot_info_t *mbd: pointer to the multiboot info structure
     * Returns: void
     */
    void physmem_init();

    // TODO: document
    void *physmem_alloc();
    void physmem_free(void *addr);
    bool physmem_is_free(void *addr);

#ifdef __cplusplus
}
#endif

#endif