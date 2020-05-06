/******************************************************************************
 * Note, the chip has 128k of flash located at addresses 0x08000000-0x0801FFFF
 *
 * I reserve the first 16k for the boot loader.
 *   0x08000000 - 0x08003FFF - Boot loader
 *   0x08004000 - 0x0801FFFF - Main program & params
 *
 *****************************************************************************/

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 0x00004000
    SRAM (rw)  : ORIGIN = 0x20000080, LENGTH = 0x00005F80
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

