/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once

#include <libdragon.h>

namespace MiMem
{
  void writeAligned(volatile uint64_t *addr, uint64_t value, int bytes);
  void zeroUnaligned(const volatile char* addr, int bytes);

  inline void write(volatile void *addr, uint64_t value, int bytes)
  {
    uint32_t misalign = ((uint32_t)addr) & 0b111;
    if(misalign) {
      asm volatile ("sdl %0, 0(%1)\n" ::"r"(value), "r"(addr) : "memory");
      addr = (char*)addr + (8  - misalign);
      bytes -= (8  - misalign);
    }
    writeAligned((volatile uint64_t*)addr, value, bytes);
  }

  inline bool isSupported()
  {
    uint64_t *addr = (uint64_t*)0xA0300000;
    addr[1] = 0xFF;
    MiMem::writeAligned(addr, 0, 16);
    return addr[1] == 0;
  }
}