bits 16
        mov dx, 0x3f8
        mov al, 0x30
        out dx, al
        mov al, 0x25
        hlt
