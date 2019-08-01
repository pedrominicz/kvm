#include <stddef.h>
#include <stdint.h>

extern const uint16_t interrupts[256]; // Defined in "entry.S".

static inline void outb(uint16_t port, uint8_t data) {
  asm volatile ("out %0, %1" :: "a"(data), "d"(port));
}

void bios_interrupt(void) {
  outb(0x3f8, 'b');
}

void bios_init(void) {
  for(size_t i = 0; i < 256; ++i) {
    asm volatile ("mov %0, %%es:(%1)" :: "r"(interrupts[i]), "r"(i * 4));
    asm volatile ("mov %0, %%es:(%1)" :: "r"(0xf000), "r"(i * 4 + 2));
  }
}
