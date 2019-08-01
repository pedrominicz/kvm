OUTPUT_ARCH("i386")
ENTRY(entry)

SECTIONS {
  .text 0 : { *(.text) }
}
