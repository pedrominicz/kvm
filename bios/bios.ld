OUTPUT_ARCH("i386")
ENTRY(entry)

SECTIONS {
  .text 0 : {
    entry.o(.text)
    *(.text)
  }

  /DISCARD/ : { *(.eh_frame*) }
}
