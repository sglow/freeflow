/******************************************************************************
 *
 *****************************************************************************/

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08010000, LENGTH = 0x00070000
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

