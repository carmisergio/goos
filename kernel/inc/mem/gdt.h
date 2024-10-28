#ifndef _MEM_GDT_H
#define _MEM_GDT_H 1

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Set up Global Descriptor Table
     */
    void setup_gdt();

#ifdef __cplusplus
}
#endif

#endif