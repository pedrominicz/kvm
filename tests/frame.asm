bits 16
        int 10
        int 11
        int 12
        mov ax, 0x1234
        mov fs, ax
        int 13
        int 14
        int 15
        hlt
