/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <libdragon.h>
#include "../miMemory.h"
#include "../math.h"
#include "../main.h"
#include "../text.h"

namespace
{
  constexpr uint16_t packColor16(color_t c) {
    return (((int)c.r >> 3) << 11) | (((int)c.g >> 3) << 6) | (((int)c.b >> 2));
  }

  constexpr uint64_t packColor(const color_t &col) {
    uint64_t res = packColor16(col);
    res = (res << 16) | res;
    return (res << 32) | res;
  }

  uint64_t getRainbowColor(uint32_t t) {
    t &= 0xFFFF;

    // Scale to hue [0, 1536) (since 6 sectors * 256 = 1536)
    // equivalent to hue in [0, 360), but we use 256 per sector
    uint32_t hue = (t * 1536) >> 16;

    uint8_t region = hue >> 8;
    uint8_t f      = hue & 0xFF;
    uint8_t q = 0xFF - f;

    switch(region) {
        case 0: return packColor({0xFF,  f  , 0x00});
        case 1: return packColor({  q , 0xFF, 0x00});
        case 2: return packColor({0x00, 0xFF,  f  });
        case 3: return packColor({0x00,  q  , 0xFF});
        case 4: return packColor({ f  , 0x00, 0xFF});
        default:
        case 5: return packColor({0xFF, 0x00,  q  });
    }
  }

  constexpr uint64_t BG_COLORS[4] = {
    packColor({50, 50, 50, 0xFF}),
    packColor({100, 100, 100, 0xFF}),
    packColor({150, 150, 150, 0xFF}),
    packColor({200, 200, 200, 0xFF}),
  };

  constinit bool isEmu{false};
}

namespace Demo::Repeat
{
  extern const char* const name = "MI-Repeat Mode";

  void init() {
    isEmu = !MiMem::isSupported();
  }

  void draw()
  {
    uint32_t rowStride = state.fb->stride;
    auto fb = (uint8_t*)state.fb->buffer;

    disable_interrupts();

    uint32_t baseLineSize = 40;
    float sinBase = 0;
    uint32_t idxStart = 0;

    uint64_t col=0;

    for(int i=0; i<240; ++i) {
      //if(i % 2 == 0)continue;
      sinBase += 0.03f;
      float offset  = Math::sinApprox(sinBase + state.time) * 0.5f;
      float offsetB = Math::sinApprox(sinBase * 5.0f + state.time*1.5f);
      float offsetC = Math::sinApprox(sinBase * 15.0f + state.time*1.7f);
      offset += offsetB * 0.2f + offsetC * 0.1f;

      uint32_t idxOffset = 200 + ((int)(offset*128) & ~0b1);
      uint32_t idx = idxStart + idxOffset;

      uint32_t lineSize = baseLineSize + (offsetB * offset * 40);
      lineSize &= ~1;

      int bytesLeft = (320 * 2) - idxOffset - lineSize;

      int stripeOff = state.time * 25;
      uint64_t colBG = BG_COLORS[((i+stripeOff)/8) & 0b11];

      col = getRainbowColor(i * 128 - state.timeInt);

      MiMem::write(&fb[idxStart],     colBG, idxOffset);
      MiMem::write(&fb[idx],          col,   lineSize);
      MiMem::write(&fb[idx+lineSize], colBG, bytesLeft);

      idxStart += rowStride;
      ++baseLineSize;
    }

    // Text
    int textPosX = 310 - (state.time * 16.0f);
    Text::printLarge(textPosX, 68, "{MI-Repeat Mode}", {
      .color = packColor({0, 0, 0, 0xFF}),
      .posCB = [](int &x, int &y, int idx){
        y = 68 + (int)(Math::sinApprox(idx * 0.9f + state.time*4) * 16.f);
      }
    });
    Text::printLarge(textPosX-4, 68, "{MI-Repeat Mode}", {
      .color = getRainbowColor(state.timeInt/2),
      .posCB = [](int &x, int &y, int idx){
        y = 68 + (int)(Math::sinApprox(idx * 0.9f + (state.time*4+0.4f)) * 16.f);
      }
    });

    if(isEmu)
    {
      Text::print(80, 100, "This`Emulator`does`NOT");
      Text::print(80, 108, "``````````````````````");
      Text::print(80, 116, "support`MI-repeat`mode");
    }

    enable_interrupts();
  }

  void destroy()
  {

  }
}