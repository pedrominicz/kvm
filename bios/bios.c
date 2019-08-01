#include <stdint.h>

void bios_init(void) {
  const uint16_t port = 0x3f8;
  asm volatile ("out %0, %1" :: "a"((uint8_t)'b'), "d"(port));
}
