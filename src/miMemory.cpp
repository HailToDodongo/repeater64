/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "miMemory.h"

void MiMem::writeAligned(volatile uint64_t *addr, uint64_t value, int bytes)
{
  do {
    *MI_MODE = MI_WMODE_SET_REPEAT | (bytes > 128 ? 127 : (bytes-1));
    *addr = value;
    bytes -= 128; // we only care about the iteration count, the size is clamped above
    addr += 128 / 8;
  } while(bytes > 0);
}

void MiMem::zeroUnaligned(const volatile char *addr, int bytes) {
  uint32_t misalign = ((uint32_t)addr) & 0b111;
  bytes += misalign;
  uint64_t value = (misalign & 0b110) ? 0xAABBCCDD'FF000000 : 0;
  do {
    asm volatile (".balign 32");
    *MI_MODE = MI_WMODE_SET_REPEAT | (bytes > 128 ? 127 : (bytes-1));
    //asm volatile ("sb $zero, 0(%0)\n" :: "r"(addr) : "memory");
    asm volatile ("sb %0, 0(%1)\n" ::"r"(value), "r"(addr) : "memory");
    bytes -= 128; // we only care about the iteration count, the size is clamped above
    addr += 128;
  } while(bytes > 0);
}
