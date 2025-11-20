/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <cmath>

#include "../main.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"
#include "../text.h"

#define DP_TEST_MODE ((volatile uint32_t*)0xA420'0004)
#define DP_BUFTEST_ADDR ((volatile uint32_t*)0xA420'0008)
#define DP_BUFTEST_DATA ((volatile uint32_t*)0xA420'000C)

namespace RDPBuff
{
  void enable() {
    *DP_TEST_MODE = 1;
  }
  void disable() {
    *DP_TEST_MODE = 0;
  }

  uint32_t read(uint32_t idx)
  {
    *DP_BUFTEST_ADDR = idx;
    MEMORY_BARRIER();
    return *DP_BUFTEST_DATA;
  }

  void write(uint32_t idx, uint32_t val)
  {
    *DP_BUFTEST_ADDR = idx;
    MEMORY_BARRIER();
    *DP_BUFTEST_DATA = val;
  }

  void clear()
  {
    for(int i=0; i<64; ++i) {
      write(i, 0);
    }
  }
}

namespace
{
  constexpr char CVG_CHAR[4][2] = {
    {'.', '\0'},
    {'-', '\0'},
    {'+', '\0'},
    {'$', '\0'},
  };

  color_t getRainbowColor(uint32_t t) {
    t &= 0xFFFF;
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

namespace Demo::RDPTestMode
{
  extern const char* const name = "RDP Test-Mode";

  void init() {
  }

  void destroy() {
  }

  void draw()
  {

    color_t prim = getRainbowColor(state.frame * 150);

    RDP::DPL dpl{128};
    dpl.add(RDP::syncPipe())
      .add(RDP::setColorImage(state.fb->buffer, RDP::Format::RGBA, RDP::BBP::_16, state.fb->stride/2))
      .add(RDP::setScissor(0, 0, state.fb->width-1, state.fb->height-1))
      .add(RDP::setOtherModes(RDP::OtherMode()
        .cycleType(RDP::CYCLE::FILL)
      ))
      .add(RDP::setFillColor({0x22, 0x22, 0x22, 0}))
      .add(RDP::fillRect(0, 0, 320-1, 240-1))

      .add(RDP::syncPipe())

      .add(RDP::setOtherModes(RDP::OtherMode()
        .cycleType(RDP::CYCLE::ONE)
        .ditherRGBA(RDP::DitherRGB::DISABLED)
        .ditherAlpha(RDP::DitherAlpha::DISABLED)
        .setAA(false)
        .forceBlend(false)
        .setImageRead(false)
        .setDepthWrite(false)
      ))
      .add(RDP::setCC(RDPQ_COMBINER1(
        (SHADE,0,PRIM,0), (0,0,0,1)))
      )
      .add(RDP::setPrimColor(prim))
      .runSync();

    constexpr float triOffset[2]{ 256, 24 };
    constexpr float size = 24;

    int pixelCount = 0;
    float h = (fm_sinf(state.frame * 0.01f)*0.5f+0.5f) * size;
    float w = (fm_sinf(state.frame * 0.02f + 2.0f)*0.5f+0.6f) * size;

    int posY = 32;
    for(int y=0; y<size; ++y)
    {
      int textPosX = 16;

      RDPBuff::enable();
      RDPBuff::clear();
      RDPBuff::disable();

      int wobbleOffset = (int)(fm_sinf(state.frame * 0.07f + y * 0.4f) * 2.4f) + 4;
      if(wobbleOffset < 0)wobbleOffset = 0;

      auto triData = RDP::triangleGen(RDP::TriAttr::SHADE, {
        .pos = {triOffset[0] + wobbleOffset, triOffset[1] + 0.0f},
        .color = {0.5f, 1, 1, 0},
      },{
        .pos = {triOffset[0] + wobbleOffset, triOffset[1] + size},
        .color = {0.25f, 0.4f, 0.2f, 0},
      },{
        .pos = {triOffset[0] + w + wobbleOffset, triOffset[1] + h},
        .color = {1, 1, 1, 0},
      });

      RDP::DPL dplTri{16};
      dplTri
        .add(RDP::setScissorExtend(0, triOffset[1]+y, SCREEN_WIDTH, 1))
        .add(RDP::triangleWrite(triData, RDP::TriAttr::SHADE))
        .runSync();

      RDPBuff::enable();

      bool isDone = false;
      color_t lastCol{0xFF, 0xFF, 0xFF};
      for(int i=0; i<32 && !isDone; i+=4)
      {
        uint32_t col[2]{RDPBuff::read(i+0), RDPBuff::read(i+1)};
        uint32_t coverage = RDPBuff::read(i+2);
        uint32_t zero = RDPBuff::read(i+3);

        color_t colors[4] {
          color_from_packed16((col[0] | zero) >> 16),
          color_from_packed16((col[0] | zero) & 0xFFFF),
          color_from_packed16((col[1] | zero) >> 16),
          color_from_packed16((col[1] | zero) & 0xFFFF),
        };

        uint8_t cvgBits[4]{
          (uint8_t)((coverage >> 6) & 0b11),
          (uint8_t)((coverage >> 4) & 0b11),
          (uint8_t)((coverage >> 2) & 0b11),
          (uint8_t)((coverage >> 0) & 0b11),
        };

        for(int j=0; j<4; ++j)
        {
          if(wobbleOffset != 0)
          {
            --wobbleOffset;
            textPosX += 8;
            continue;
          }

          if(color_to_packed32(colors[j]) == 0)isDone = true;
          uint8_t cvg = cvgBits[j];

          if(colors[j].r > lastCol.r)isDone = true;
          if(colors[j].g > lastCol.g)isDone = true;
          if(colors[j].b > lastCol.b)isDone = true;
          lastCol = colors[j];

          if(!isDone)
          {
            Text::setColor(colors[j]);
            Text::printf(textPosX, posY, cvg < 4 ? CVG_CHAR[cvg] : "?");
            Text::setColor();
            textPosX += 8;
            ++pixelCount;
          }

          if(colors[j].a != 0 && cvg != 3)isDone = true;
          if(!isDone && colors[j].a == 0)
          {
            isDone = true;
            textPosX -= 8;
          }
        }
      } // span

      posY += 8;
    }

    Text::setColor();

    // Write test, this simply checks if the region is writable
    // and handled masking correctly

    RDPBuff::write(0, 0xDEAD'BEEF);
    RDPBuff::write(1, 0x1234'5678);
    RDPBuff::write(2, 0xDEAD'BEEF);
    RDPBuff::write(3, 0x1234'5678);

    bool pass0 = RDPBuff::read(0) == 0xDEAD'BEEF;
    bool pass1 = RDPBuff::read(1) == 0x1234'5678;
    bool pass2 = RDPBuff::read(2) == 0x0000'00EF;
    bool pass3 = RDPBuff::read(3) == 0x0000'0000;

    Text::printf(80, 16, "Mask-R/W: %s %s %s %s",
      pass0 ? "OK" : "FAIL", pass1 ? "OK" : "FAIL",
      pass2 ? "OK" : "FAIL", pass3 ? "OK" : "FAIL"
    );

    Text::setColor({0xFF, 0x99, 0x99});
    if(!pass0)Text::printf(208, 48+8,  "0: %08X", RDPBuff::read(0));
    if(!pass1)Text::printf(208, 48+16, "1: %08X", RDPBuff::read(1));
    if(!pass2)Text::printf(208, 48+24, "2: %08X", RDPBuff::read(2));
    if(!pass3)Text::printf(208, 48+32, "3: %08X", RDPBuff::read(3));
    if(pixelCount < 16)Text::printf(208, 48+48, "Span: FAIL", pixelCount);
    Text::setColor();

    RDPBuff::disable();
  }
}