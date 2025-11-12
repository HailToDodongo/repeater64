/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <cmath>
#include "demoSync.h"
#include "../main.h"
#include "../math.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"

namespace {
  constinit sprite_t *bg0{};
  constinit sprite_t *bg1{};
  constinit bool doWaterFX{};
}

void Demo::Sync::init()
{
  bg0 = sprite_load("rom:/bg0.rgba16.sprite");
  bg1 = sprite_load("rom:/bg1.rgba16.sprite");
  doWaterFX = true;
}

void Demo::Sync::destroy() {
  if(bg0)sprite_free(bg0);
  if(bg1)sprite_free(bg1);
}

void Demo::Sync::draw()
{
  RDP::DPL dpl{8};
  dpl.add(RDP::syncPipe())
    .add(RDP::setColorImage(state.fb->buffer, RDP::Format::RGBA, RDP::BBP::_16, state.fb->stride/2))
    .add(RDP::setScissor(0, 0, state.fb->width-1, state.fb->height-1))

    // clear screen
    .add(RDP::setOtherModes(RDP::OtherMode().cycleType(RDP::CYCLE::FILL)))
    //.add(RDP::setFillColor({0x10, 0x10, 0x10, 0}))
    //.add(RDP::fillRect(0, 0, state.fb->width-1, state.f->height-1))
    .runSync();
    /*
    .add(RDP::setOtherModes(RDP::OtherMode()
      .cycleType(RDP::CYCLE::ONE)
      .ditherRGBA(RDP::DitherRGB::DISABLED)
      .ditherAlpha(RDP::DitherAlpha::DISABLED)
      .setAA(false)
      .forceBlend(false)
    ))
    .add(RDP::setCC(RDPQ_COMBINER1(
      (0,0,0,ENV), (0,0,0,ENV)))
    );
*/

/*
  RDP::DPL dplTest{1000};
 int posX = 20;
  int posY = 32;
  for(int i=0; i<12; ++i)
  {
    int end = i;
    int offsetX = posX;
    for(int h=0; h<=14; ++h)
    {
      dplTest.add(RDP::setFillColor({0x70, 0x70, 0x70, 0xFF}))
        .add(RDP::fillRect(offsetX, posY, offsetX+end, posY + h))
        .add(RDP::setFillColor({0xFF, 0x00, 0x00, 0xFF}))
        .add(RDP::syncPipe());

      offsetX += 18;
    }

    posY += 16;
  }
  dplTest.runAsync();
  return;
*/

  auto data0 = (uint32_t*)(sprite_get_pixels(bg0).buffer);
  auto data1 = (uint32_t*)(sprite_get_pixels(bg1).buffer);

  RDP::DPL rowDpl[2]{
    {320*4 + 2}, {320*4 + 2}
  };

  int8_t offsetY[32];

  for(int x=0; x<32; ++x)
  {
    offsetY[x] = Math::sinApprox(x/5.0f + state.time*1.5f) * 4.0f;
  }

  for(int x=0; x<160; ++x)
  {
    // prefill commands
    rowDpl[0].add(RDP::setFillColorRaw(0))
        .add(RDP::fillRectFP(0,0,0,0))
        .add(RDP::setFillColorRaw(0))
        .add(RDP::syncPipe());

    rowDpl[1].add(RDP::setFillColorRaw(0))
        .add(RDP::fillRectFP(0,0,0,0))
        .add(RDP::setFillColorRaw(0))
        .add(RDP::syncPipe());
  }

  int posMask = 0xFFFF;
  int posShift = 0;

  auto posToIndex = [&posShift, &posMask](int x, int y) {
    x += posShift;
    x &= posMask;
    y += posShift;
    y &= posMask;
    if(y > 239)y = 239;
    return (y * 320 + x) / 2;
  };

  int waterHeight = doWaterFX ? 160 : 240;
  int dplIdx = 0;

  float pixelate = Math::sinApprox(state.time * 0.4f) * 12 - 5;
  pixelate = pixelate < 0 ? 0 : pixelate;

  posMask = 0xFFFF << (int)pixelate;
  posShift = (1 << (int)pixelate) / 2;

  int skipIdx = state.frame % 4;
  for(int y=skipIdx; y<240; y+=4)
  {
    RDP::DPL &currDPL = rowDpl[dplIdx];
    dplIdx ^= 1;
    currDPL.reset();

    int offsetX = ((y * 240) ^ (y*128)) >> 7;
    offsetX += TICKS_READ() & 0b1;

    for(int x=0; x<320; x+=2)
    {
      int sampleY = y + (y > waterHeight ? (offsetY[(x/2+offsetX) & 31]) : 0);

      int idx = posToIndex(x, sampleY);

      auto dpl32 = (uint32_t*)currDPL.dplEnd;
      dpl32[1] = RDP::setFillColorRaw(data1[idx]);
      currDPL.dplEnd[1] = RDP::fillRectFP(x*4, y*4, x*4+4, y*4);
      dpl32[5] = RDP::setFillColorRaw(data0[idx]);
      currDPL.dplEnd += 4;
    }

    currDPL.runAsync();
  }

  // while the effect above in itself "detects" an emu, also do it here to disable water
  // this makes the text in the emu image readable
  doWaterFX = (((uint16_t*)state.fb->buffer)[100] & 0b1110) != 0;
}
