/******************************************************************************
 * Note, the chip has 128k of flash located at addresses 0x08000000-0x0801FFFF
 * I'm allocating 96k to code and reserving 32k for other purposes:
 *   0x08000000 - 0x08017FFF - code and constants
 *   0x08018000 - 0x0801EFFF - reserved for future use
 *   0x0801F000 - 0x0801FFFF - non-volatile parameter storage
 *
 * We have 40k of SRAM: 0x20000000 - 0x20009FFF
 *
 * For now I'm reserving a big chunk of that (the last 16k) for the trace buffer
 * Also, I use the first 128 bytes as debugging scratchpad data, so the SRAM
 * section defined below is 0x20000080 - 0x20005FFF
 *
 * If things get tight I'll just reduce the trace buffer size, but it's so handy
 * for development / debug that I'm keeping it big for now.
 *****************************************************************************/

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 0x00018000
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

