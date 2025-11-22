/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "rdpDumpTest.h"
#include "main.h"
#include "rdp/rdp.h"
#include "rdp/dpl.h"

#include "text.h"

namespace
{
}

void RDPDumpTest::run(std::function<void(uint32_t)> fn)
{
   // detect if we are crashed
    RDP::DPL dplTest{8};
    dplTest.add(RDP::syncPipe())
      .add(RDP::syncFull())
      .runAsyncUnsafe();

    wait_ticks(TICKS_FROM_US(250));

    if(*DP_STATUS & DP_STATUS_PIPE_BUSY)
    {
      int posY = 64;
      Text::setColor({0xFF, 0x22, 0x22});
      Text::printf(64, posY, "!!! RDP HAS CRASHED !!!"); posY += 8;
      Text::setColor();
      Text::printf(64, posY, "Please Power Cycle"); posY += 16;

      Text::printf(64, posY, "DP_CLCK: %08X", *DP_CLOCK); posY += 8;
      Text::printf(64, posY, "DP_BUSY: %08X", *DP_BUSY); posY += 8;
      Text::printf(64, posY, "DP_CURR: %08X", *DP_CURRENT); posY += 8;
      Text::printf(64, posY, "DP_END : %08X", *DP_END); posY += 8;
      return;
    }

    uint32_t testDoneCount = 0;
    uint32_t testSuccessCount = 0;
    for(auto res : testRes) {
      if(res != 0xFFFF'FFFF)++testDoneCount;
      testSuccessCount += res == 0 ? 1 : 0;
    }
    if(testDoneCount == testCases.size()) {
      autoMode = false;
    }

    auto held = joypad_get_inputs(JOYPAD_PORT_1);
    auto pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if(pressed.c_right || pressed.d_right)++testIdx;
    if(pressed.c_left || pressed.d_left)testIdx = (testIdx + testCases.size() - 1) % testCases.size();
    if(autoMode)testIdx++;
    testIdx = testIdx % testCases.size();


    fn(testCases[testIdx]);

    // Load referende file
    const char* filePath = "rom:/00000000.test";
    sprintf((const_cast<char*>(filePath) + 5), "%08lX.test", testCases[testIdx]);

    uint16_t *testDataOrg = nullptr;

    // comment-in to avoid testing:
    testDataOrg = (uint16_t *)asset_load(filePath, nullptr);

    auto testData = testDataOrg ? testDataOrg : (uint16_t*)&testCases;

    bool isDump = pressed.b;
    if(isDump)debugf("TEST=%08X\n", testCases[testIdx]);

    // go pixel by pixel in the test region and compare
    // if pressed B, dump the framebuffer over debugf
    uint16_t* fbPtr = (uint16_t*)state.fb->buffer;
    fbPtr += testRegion[1] * (state.fb->stride / 2);

    testRes[testIdx] = 0;
    int totalPixel = 0;

    for(int y=testRegion[1]; y<testRegion[3]; ++y)
    {
      for(int x=testRegion[0]; x<=testRegion[2]; ++x)
      {
        uint16_t col = fbPtr[x];
        if(isDump)debugf("%04X", col);

        testRes[testIdx] += (*testData != col) ? 1 : 0;
        ++testData;
        totalPixel += (col == 0x2108) ? 0 : 1; // ignore BG pixels
      }
      fbPtr += (state.fb->stride / 2);
      if(isDump)debugf("\n");
    }

    if(testDataOrg)free(testDataOrg);

    // prints results per test at the bottom
    int py = 200;
    int px = 16;
    Text::printf(px, py, "Errors:         (Test: %02d|%08X)", testIdx, testCases[testIdx]);
    py+=10;
    for(uint32_t t=0; t<testCases.size(); ++t)
    {
      if(testRes[t] == 0xFFFF'FFFF) {
        Text::setColor({0x99, 0x99, 0x99});
        Text::printf(px, py, "---");
      } else {
        Text::setColor(
          testRes[t] == 0 ? color_t{0x66, 0xFF, 0x66} : color_t{0xFF, 0x66, 0x66}
        );
        Text::printf(px, py, "%03X", testRes[t]);
      }
      Text::setColor();
      px += 28;
      if((t+1) % 10 == 0) {
        py+=8;
        px = 16;
      }
    }

    // Test results on top
    py = 32;
    if(testDoneCount == testCases.size()) {
      Text::printf(16, py, "Passed: %d/%d", testSuccessCount, testCases.size());
      px = 128;
      if(testSuccessCount == testCases.size()) {
        Text::setColor({0x66, 0xFF, 0x66});
        Text::print(px, py, "OK");
      } else {
        Text::setColor({0xFF, 0x66, 0x66});
        Text::print(px, py, "FAIL!");
      }
    } else {
      Text::print(16, py, "Test running...");
    }
    Text::setColor();

    wait_ms(5);
}
