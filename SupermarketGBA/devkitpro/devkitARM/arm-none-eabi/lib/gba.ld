OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
    ewram (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
    iwram (rwx) : ORIGIN = 0x03000000, LENGTH = 32K
}

SECTIONS {
    .text 0x08000000 : {
        KEEP(*(.crt0))
        *(.text.start) *(.text) *(.text.*) *(.gnu.linkonce.t.*)
        *(.rodata) *(.rodata.*) *(.gnu.linkonce.r.*)
        . = ALIGN(4);
        __text_end = .;
    } > rom

    .ewram (NOLOAD) : {
        . = ALIGN(4);
        *(.ewram) *(.ewram.*)
    } > ewram

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss) *(.bss.*) *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > iwram

    .data : AT (__text_end) {
        . = ALIGN(4);
        __data_start = .;
        *(.data) *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } > iwram

    /DISCARD/ : { *(.note*) *(.comment*) }
}
