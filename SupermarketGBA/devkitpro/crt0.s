    .section .crt0, "ax"
    .arm
    .global _start

_start:
    b _real_start

    @ GBA ROM header placeholder (188 bytes)
    .space 188

_real_start:
    @ Set IRQ stack
    mov r0, #0xD2
    msr cpsr, r0
    ldr sp, =0x03007FA0

    @ Set SYS stack
    mov r0, #0xDF
    msr cpsr, r0
    ldr sp, =0x03008000

    @ Zero BSS
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
_bss_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt _bss_loop

    @ Call main
    bl main

_hang:
    b _hang
