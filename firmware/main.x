/******************************************************************************
 * Note, the chip has 128k of flash located at addresses 0x08000000-0x0801FFFF
 * I'm allocating 96k to code and reserving 32k for other purposes:
 *   0x08000000 - 0x08017FFF - code and constants
 *   0x08018000 - 0x0801EFFF - reserved for future use
 *   0x0801F000 - 0x0801FFFF - non-volatile parameter storage
 *****************************************************************************/

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 0x00018000
    SRAM (rw)  : ORIGIN = 0x20000080, LENGTH = 0x00009F80
}

SECTIONS
{
    .text :
    {
        _text = .;
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        _data = .;
        *(vtable)
        *(.data*)
        _edata = .;
    } > SRAM

    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM
}

