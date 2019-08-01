OUTPUT_ARCH("i386")
ENTRY(entry)

SECTIONS {
  .text 0 : {
    entry.o(.text)
    entry.o(.data)
    *(.text)
  }

  /DISCARD/ : { *(.eh_frame*) }
}
