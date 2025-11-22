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
  constinit RDPDumpTest dumpTest{
    .testCases = std::array<uint32_t, 20>({
      0,1,2,3,4,5,6,7,8,9,10,
      11,12,13,14,15,16,17,18,19,
    }),
    .testRegion = std::array<int, 4>({
      16, 48, SCREEN_WIDTH-16, SCREEN_HEIGHT-48
    })
  };

  float lastY = 0;
}

namespace Demo::RDPNoSync1C
{
  extern const char* const name = "RDP 1-Cycle No-Sync";

  void init() {
    for(uint32_t t=0; t<dumpTest.testCases.size(); ++t) {
      dumpTest.testCases[t] = 0x1000'0000 | t;
    }
    dumpTest.reset();
  }

  void destroy() {}

  void draw()
  {
    lastY = 0;
    dumpTest.run([](uint32_t testCase)
    {
      testCase = testCase & 0xFF;
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

      RDP::DPL dplTri{2000};
      dplTri.add(RDP::syncPipe())
        .add(RDP::setFillColor({0x22, 0x22, 0x22, 0}))
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
          (0,0,0,ENV), (0,0,0,1)))
        );

        float baseSizeY = (testCase*6)+1;
        float baseSizeX = 1;
        if(testCase >= 10)
        {
          baseSizeX = ((testCase-10)*10)+1;
          baseSizeY = 8;
        }

        float pos[2]{0.0f, dumpTest.testRegion[1]+2.0f};
        for(int y=0;  y<32; ++y)
        {
          pos[0] = dumpTest.testRegion[0]+2;
          for(int x=0;  x<64; ++x)
          {
            dplTri
              .add(RDP::syncPipe())
              .add(RDP::setEnvColor({0xFF, 0xFF, 0xFF, 0xFF}))
              .add(RDP::fillRectSize(
                pos[0], pos[1], x+baseSizeX, y+baseSizeY
              ))
              .add(RDP::setEnvColor({0xFF, 0x00, 0x00, 0xFF}))
              .add(RDP::setEnvColor({0x00, 0xFF, 0x00, 0xFF}))
              .add(RDP::setEnvColor({0x00, 0x00, 0xFF, 0xFF}))
            ;
            pos[0] += x+baseSizeX+1;

            if(pos[0] + x + baseSizeX + 1 > dumpTest.testRegion[2]) {
              break;
            }
          }
          pos[1] += y+baseSizeY+1;
          if((pos[1]+y+baseSizeY+1) > dumpTest.testRegion[3]) {
            break;
          }
        }

        dplTri.runSync(TICKS_FROM_MS(100));
    });
  }
}