/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <cmath>
#include "../main.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"
#include "../text.h"

#define ICACHE_LINESIZE 0x20

extern "C" {
  /**
  *  Copy `num_words` from hidden RDRAM bits associated with address `src` into `dst`.
  *  Original source from: Thar0
  */
  __attribute__((noinline))
  __attribute__((aligned(ICACHE_LINESIZE)))
  void ebusReadBytes(uint8_t* dst, uint32_t* src, size_t numBytes)
  {
    assert(numBytes % 4 == 0);

    // Pre-fill the cache as ebus test mode changes all RDRAM accesses, cache fetches
    // lock up the system while it is enabled.
    for (char* cur = (char*)(void*)ebusReadBytes; cur < &&__func_end; cur += ICACHE_LINESIZE)
    {
      __asm__ ("cache 0x14, (%0)" : : "r"(cur));
    }

    size_t  numWords = numBytes / 4;
    for (size_t i = 0; i < numWords; i++)
    {
      uint32_t ebus_read;

      // extract hidden bits for this word, appearing as the 4 low-order bits
      *MI_MODE = MI_WMODE_SET_EBUS;
      MEMORY_BARRIER();
      ebus_read = src[i];
      MEMORY_BARRIER();
      *MI_MODE = MI_WMODE_CLR_EBUS;
      MEMORY_BARRIER();

      // intentionally preserve the rest of the bits,
      // this should be zero on hardware
      dst[0] = (ebus_read >> 3) & 0b11111111'11110001;
      dst[1] = (ebus_read >> 2) & 0b11111111'11110001;
      dst[2] = (ebus_read >> 1) & 0b11111111'11110001;
      dst[3] = (ebus_read >> 0) & 0b11111111'11110001;
      dst += 4;
    }
    __func_end:;
  }
}

namespace
{
  /**
   * Writes a 16bit value to RDRAM, while setting the upper hidden bit to the given state.
   * The lower hidden bit is destroyed.
   * @param dst
   * @param value visible RDRAM value
   * @param hiddenBit desired state (0 or 1)
   */
  void writeHiddenU16(volatile uint16_t *dst, uint16_t value, uint8_t hiddenBit)
  {
    MEMORY_BARRIER();
    if(hiddenBit) {
      *dst = (value | 1); // LSB=1 forces both hidden-bits to be one
    } else {
      *dst = value & 0xFF00;  // LSB=0 forces both hidden-bits to be zero
    }
    MEMORY_BARRIER();
    // set correct low byte, destroys low hidden-bit, preserves upper
    ((volatile uint8_t*)dst)[1] = (uint8_t)value;
  }

  constinit sprite_t* bg{nullptr};

  constexpr uint32_t HIDDEN_MASK_COUNT = 110;
  constexpr uint32_t HIDDEN_MASK_COUNT_SAFE = 128;

  __attribute__((aligned(8)))
  constinit uint16_t *hiddenMask{nullptr};
}

namespace Demo::HiddenBits
{
  extern const char* const name = "RDRAM 9th Bit";

  void init() {
    bg = sprite_load("rom:/bgBits.rgba16.sprite");
    hiddenMask = static_cast<uint16_t*>(malloc_uncached(HIDDEN_MASK_COUNT_SAFE * sizeof(uint16_t)));

    uint8_t rngInvert[HIDDEN_MASK_COUNT]{};

    for(uint32_t i=0; i<HIDDEN_MASK_COUNT; ++i) {
      uint16_t rngMask = rand() & 0xFFFF;
      rngInvert[i] = rand() & 1;
      writeHiddenU16(&hiddenMask[i], rngMask, rngInvert[i]);
    }

    auto imgData = (uint16_t*)bg->data;
    for(uint32_t i=0; i<(bg->width*bg->height); ++i) {
      uint32_t idxMask = i % HIDDEN_MASK_COUNT;
      uint16_t mask = hiddenMask[idxMask];
      if(rngInvert[idxMask])mask = ~mask;
      imgData[i] ^= mask;
    }
  }

  void destroy() {
    sprite_free(bg);
    bg = nullptr;
    free_uncached(hiddenMask);
    hiddenMask = nullptr;
  }

  void draw()
  {
    RDP::DPL dpl{1400};
    dpl.add(RDP::syncPipe())
      .add(RDP::setColorImage(state.fb->buffer, RDP::Format::RGBA, RDP::BBP::_16, state.fb->stride/2))
      .add(RDP::setScissor(0, 0, state.fb->width-1, state.fb->height-1))
      .add(RDP::setOtherModes(RDP::OtherMode()
        .cycleType(RDP::CYCLE::FILL)
      ))
      .add(RDP::setFillColorRaw(0xFFF0FFF0)) // clears hidden bits (LSB=0)
      .add(RDP::fillRect(0, 0, 320-1, 240-1))

      .add(RDP::syncPipe())
      .add(RDP::setFillColor(color_t{0x70, 0x70, 0x30, 0xFF})) // sets hidden bits to 1 (LSB=1)
    ;

    for(int y=32; y<240-32; y+=2) {
      dpl.add(RDP::fillRect(0, y, SCREEN_WIDTH-1, y));
    }
    dpl.runSync();


    auto *buff = static_cast<uint32_t*>(state.fb->buffer);
    buff += state.fb->stride * 32 / 4; // skip first 32 rows

    uint8_t bits[SCREEN_WIDTH*2];
    uint8_t maskBits[HIDDEN_MASK_COUNT_SAFE*2]{};

    disable_interrupts();
      ebusReadBytes(maskBits, (uint32_t*)hiddenMask, HIDDEN_MASK_COUNT_SAFE*2);
    enable_interrupts();

    constexpr uint32_t RNG_SHIFT = 32;
    uint16_t *imgIn = (uint16_t*)bg->data;
    imgIn -= RNG_SHIFT;

    uint32_t cpuMaskIdx = 0;
    uint16_t maskInv = 0;
    for(uint32_t y=32; y<240-32; ++y)
    {
      disable_interrupts();
        ebusReadBytes(bits, buff, SCREEN_WIDTH*2);
      enable_interrupts();

      int isOdd = y & 1;

      volatile uint16_t *buffCol = (volatile uint16_t*)buff;

      uint8_t *bitsIn = bits;
      for(uint32_t x=0; x<SCREEN_WIDTH; ++x)
      {
        uint32_t offsetImg = bitsIn[0] * RNG_SHIFT;
        offsetImg += isOdd * RNG_SHIFT;

        uint32_t rng = C0_COUNT() * (bitsIn[0] >> 1);

        uint16_t col = (bitsIn[0] * 0xFF) << 8;
        col |= bitsIn[1] * 0xFF;
        col ^= maskInv;
        col |= rng;

        uint16_t maskBitInv = maskBits[cpuMaskIdx*2];
        uint16_t cpuMaskVal = maskBitInv ? ~hiddenMask[cpuMaskIdx] : hiddenMask[cpuMaskIdx];

        uint16_t imgData = imgIn[offsetImg] ^ cpuMaskVal;

        col ^= (0xFFFF - imgData);

        *buffCol = col;
        ++buffCol;
        bitsIn += 2;
        ++imgIn;

        cpuMaskIdx = (cpuMaskIdx + 1) % HIDDEN_MASK_COUNT;
      }

      buff += state.fb->stride / 4;
      maskInv ^= 0xFFFF;
    }
  }
}