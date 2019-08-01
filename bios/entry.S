.org 0
.code16gcc

.text
.global entry
entry:
        cli
        mov $0xf000, %ax
        mov %ax, %ds
        mov %ax, %es
        mov %ax, %ss

        mov $entry, %esp

        call bios_init

        xor %dl, %dl
        int $0x19

        ljmp $0x7c0, $0
