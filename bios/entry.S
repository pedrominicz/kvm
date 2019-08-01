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
        hlt

        xor %dl, %dl
        int $0x19

        ljmp $0x7c0, $0
