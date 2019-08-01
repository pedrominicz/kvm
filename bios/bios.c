#include <stddef.h>
#include <stdint.h>

typedef struct InterruptFrame {
  // Pushed by interrupt entry.
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint16_t gs;
  uint16_t fs;
  uint16_t es;
  uint16_t ds;
  uint32_t interrupt_number;
  // Pushed by the CPU.
  uint16_t ip;
  uint16_t cs;
  uint32_t flags;
} InterruptFrame;

extern const uint16_t interrupts[256]; // Defined in "entry.S".

static inline void outb(uint16_t port, uint8_t data) {
  asm volatile ("out %0, %1" :: "a"(data), "d"(port));
}

void bios_interrupt(InterruptFrame* frame) {
  if(frame->fs == 0x1234)
    outb(0x3f8, 'b');
  else
    outb(0x3f8, 'a');
}

void bios_init(void) {
  for(size_t i = 0; i < 256; ++i) {
    asm volatile ("mov %0, %%es:(%1)" :: "r"(interrupts[i]), "r"(i * 4));
    asm volatile ("mov %0, %%es:(%1)" :: "r"(0xf000), "r"(i * 4 + 2));
  }
}
