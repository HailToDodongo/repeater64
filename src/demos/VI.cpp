/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <cmath>
#include "../main.h"
#include "../math.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"
#include "../text.h"

extern "C" {
  void vi_debug_dump(int verbose);
}

namespace {
  color_t getRainbowColor(uint32_t t) {
    t &= 0xFFFF;

    // Scale to hue [0, 1536) (since 6 sectors * 256 = 1536)
    // equivalent to hue in [0, 360), but we use 256 per sector
    uint32_t hue = (t * 1536) >> 16;

    uint8_t region = hue >> 8;
    uint8_t f      = hue & 0xFF;
    uint8_t q = 0xFF - f;

    switch(region) {
        case 0: return {0xFF,  f  , 0x00};
        case 1: return {  q , 0xFF, 0x00};
        case 2: return {0x00, 0xFF,  f  };
        case 3: return {0x00,  q  , 0xFF};
        case 4: return { f  , 0x00, 0xFF};
        default:
        case 5: return {0xFF, 0x00,  q  };
    }
  }
}

namespace Demo::VI
{
  extern const char* const name = "VI Per-Line Effects";

  void init()
  {

  }

  void destroy() {

  }

  void draw()
  {
    RDP::DPL dpl{2000};
    dpl.add(RDP::syncPipe())
      .add(RDP::setColorImage(state.fb->buffer, RDP::Format::RGBA, RDP::BBP::_16, state.fb->stride/2))
      .add(RDP::setScissor(0, 0, state.fb->width-1, state.fb->height-1))
      .add(RDP::setOtherModes(RDP::OtherMode()
        .cycleType(RDP::CYCLE::FILL)
      ));
    //.runSync();


    for(int y=0; y<240-16; y+=16) {
      bool odd = (y & 16) == 0;
      auto color = getRainbowColor(y* 200);
      for(int x=0; x<320; x+=16) {
        odd = !odd;
        dpl.add(RDP::syncPipe())
          .add(RDP::setFillColor(
          odd ? color
              : color_t{0x30, 0x30, 0x30, 0xFF}
          ))
          .add(RDP::fillRect(x, y, x+16, y+16));
      }
    }
    dpl.runAsync();

    uint32_t orgXScale = *VI_X_SCALE;
    uint32_t orgHVideo = *VI_H_VIDEO;

    uint32_t lastLine = *VI_V_CURRENT;

    if(state.frame < 3)return;

    //vi_debug_dump(1);

    constexpr int startLine = 20;
    int scaleA = 0;
    int scaleB = 0;

    while(lastLine < 486) {
      uint32_t scanLine = *VI_V_CURRENT;
      if(scanLine != lastLine) {
        if(lastLine > startLine) {

          int pixelStart = 110 + scaleA;
          int pixelEnd = 740;

          scaleB *= 2;
          scaleB += scaleA;

          MEMORY_BARRIER();
          *VI_X_SCALE = (0x200 + scaleB + 40);
          *VI_H_VIDEO = pixelEnd | (pixelStart << 16);
          MEMORY_BARRIER();

          scaleA = Math::sinApprox(((scanLine+1) / 60.0f) + state.time) * 50 + 50;
          scaleB = Math::sinApprox(((scanLine+1) / 50.0f) - state.time*1.3f) * 30 + 30;

        } else {
          MEMORY_BARRIER();
          *VI_X_SCALE = 0x10;
          *VI_H_VIDEO = 0;
          MEMORY_BARRIER();
        }
        lastLine = scanLine;
      }
    }

    MEMORY_BARRIER();
    *VI_X_SCALE = orgXScale;
    *VI_H_VIDEO = orgHVideo;
    MEMORY_BARRIER();

    Text::print(52, 100, "This`screen`should`~wobble~");

    Text::print(32, 240-16, "Per-Line VI_X_SCALE / VI_H_VIDEO");
  }
}