#include <stddef.h>
#include <stdint.h>

void bios_init(void) {
  for(size_t i = 0; i < 256; ++i) {
    asm volatile ("mov %0, %%es:(%1)" :: "r"(0x1234), "r"(i * 4));
    asm volatile ("mov %0, %%es:(%1)" :: "r"(0xabcd), "r"(i * 4 + 2));
  }
}
