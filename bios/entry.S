.code16gcc

.text
.global entry
entry:
        cli
        mov $0xf000, %ax
        mov %ax, %ds
        mov %ax, %ss

        xor %ax, %ax
        mov %ax, %es

        mov $entry, %esp

        call bios_init

        xor %dl, %dl
        int $0x19

        ljmp $0x7c0, $0

interrupt_common:
        push %ds
        push %es
        push %fs
        push %gs
        pusha

        push %esp
        call bios_interrupt
        add $4, %esp

        popa
        pop %gs
        pop %fs
        pop %es
        pop %ds
        add $4, %esp

        .byte 0xcf

.macro interrupt n
.text
interrupt_\n:
        push $\n
        jmp interrupt_common
.data
        .word interrupt_\n
.endm

.data
.global interrupts              // `const uint16_t interrupts[256]`
interrupts:
.altmacro
.set i, 0
.rept 256
        interrupt %i
        .set i, i + 1
.endr
