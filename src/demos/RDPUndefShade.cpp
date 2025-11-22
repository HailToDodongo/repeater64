/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "../main.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"
#include "../rdpDumpTest.h"

#include <array>
#include <cmath>

namespace
{
  constinit uint32_t seed = 0x12345678;
  uint32_t fixedRand()
  {
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed;
  }

  float fixedRandf() {
    return ((fixedRand() & 0xFFFF) / (float)0xFFFF);
  }

  constinit RDPDumpTest dumpTest{
    .testCases = std::array<uint32_t, 20>({
      0x949db639u, 0x79e6622eu, 0xbff1a7abu, 0xb97daba5u, 0xf029c740u,
      0xc221ef19u, 0x3e5e8f05u, 0x40cb07a2u, 0x32c9921au, 0x84f09d97u,
      0x77107f77u, 0x63a04322u, 0x2804ac62u, 0x3419e23au, 0x33fd636fu,
      0x563c6060u, 0x856d5300u, 0xdf19d3cfu, 0x1fc4b690u, 0x5c2f3fceu,
    }),
    .testRegion = std::array<int, 4>({
      16, 48, SCREEN_WIDTH-16, SCREEN_HEIGHT-48
    })
  };
}

namespace Demo::RDPUndefShade
{
  extern const char* const name = "RDP Undefined Shade";

  void init() {
    dumpTest.reset();
  }

  void destroy() {}

  void draw()
  {
    dumpTest.run([](uint32_t testCase)
    {
      seed = testCase;

      RDP::DPL dpl{128};
      dpl.add(RDP::syncPipe())
        .add(RDP::setColorImage(state.fb->buffer, RDP::Format::RGBA, RDP::BBP::_16, state.fb->stride/2))
        .add(RDP::setScissor(0, 0, state.fb->width-1, state.fb->height-1))
        .add(RDP::setOtherModes(RDP::OtherMode()
          .cycleType(RDP::CYCLE::FILL)
        ))
        .add(RDP::setFillColor({0, 0, 0, 0}))
        .add(RDP::fillRect(0, 0, 320-1, 240-1))
        .runSync();

      float triPos[3][2]{
        {fixedRandf() * 250.0f - 100.0f, fixedRandf() * 360.0f},
        {fixedRandf() * 450.0f, fixedRandf() * 260.0f},
        {fixedRandf() * 450.0f, fixedRandf() * 360.0f}
      };

      RDP::DPL dplTri{128};
      dplTri.add(RDP::syncPipe())
        .add(RDP::setFillColor({0x11, 0x11, 0x22, 0}))
        .add(RDP::setScissor(dumpTest.testRegion[0], dumpTest.testRegion[1], dumpTest.testRegion[2], dumpTest.testRegion[3]))
        .add(RDP::fillRect(dumpTest.testRegion[0], dumpTest.testRegion[1], dumpTest.testRegion[2], dumpTest.testRegion[3]))
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
          (0,0,0,SHADE), (0,0,0,1)))
        )
        .add(RDP::triangle(0,
          {.pos = {triPos[0][0], triPos[0][1]}},
          {.pos = {triPos[1][0], triPos[1][1]}},
          {.pos = {triPos[2][0], triPos[2][1]}}
         ))
        .runSync(TICKS_FROM_MS(100));
    });
  }
}