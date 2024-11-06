MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    BOOTLOADER (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    APPLICATION (rx) : ORIGIN = 0x08010000, LENGTH = 448K
}
SECTIONS
{
    .bootloader :
    {
        . = ALIGN(4);
        _boot_start = .;
        KEEP(*(.isr_vector)) /* Interrupt vectors */
        KEEP(*(.text.bootloader)) /* Bootloader code */
        . = ALIGN(4);
        _boot_end = .;
    } > BOOTLOADER

    .text :
    {
        . = ALIGN(4);
        _stext = .;
        *(.text)
        *(.text*)
        . = ALIGN(4);
        _etext = .;
    } > APPLICATION

    .data :
    {
        _sdata = .;
        _la_data = LOADADDR(.data); /* Define _la_data */
        *(.data)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> APPLICATION

    .bss :
    {
        _sbss = .;
        *(.bss)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM
}