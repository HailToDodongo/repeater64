/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "../main.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"
#include "../text.h"

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

  float fixedRandfs() {
    return ((fixedRand() & 0xFFFF) / 32768.0f) - 1.0f;
  }

  constexpr auto TEST_CASES = std::to_array({
    0x73bfd1a2u, 0x3809113eu, 0x17d6a1e3u, 0x392df0aau, 0x0f4529b0u,
    0x38bb9993u, 0x8216fc78u, 0x06868e65u, 0x255206b1u, 0x59b323c8u,
    0x6f2489a4u, 0xa2aaae1du, 0xb6a33922u, 0x0a5212bbu, 0xf2dcebddu,
    0xe58b24beu, 0x7be66493u, 0x9f605ee7u, 0x336a3a0au, 0xa3a56459u,
  });

  constexpr int TEST_REGION[4]{
    16, 48, SCREEN_WIDTH-16, SCREEN_HEIGHT-48
  };

  constinit std::array<uint32_t, TEST_CASES.size()> TEST_RES{};

  constinit uint32_t testIdx = 0;
  constinit bool autoMode = false;
}

namespace Demo::RDPFillTri
{
  extern const char* const name = "RDP Fill-Mode Triangles";

  void init() {
    testIdx = 0;
    TEST_RES.fill(0xFFFF'FFFF);
    autoMode = true;
  }

  void destroy() {}

  void draw()
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
    for(auto res : TEST_RES) {
      if(res != 0xFFFF'FFFF)++testDoneCount;
      testSuccessCount += res == 0 ? 1 : 0;
    }
    if(testDoneCount == TEST_CASES.size()) {
      autoMode = false;
    }

    auto held = joypad_get_inputs(JOYPAD_PORT_1);
    auto pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    if(pressed.c_right || pressed.d_right)++testIdx;
    if(pressed.c_left || pressed.d_left)testIdx = (testIdx + TEST_CASES.size() - 1) % TEST_CASES.size();
    if(autoMode)testIdx++;
    testIdx = testIdx % TEST_CASES.size();

    seed = TEST_CASES[testIdx];

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
      {fixedRandfs() * 150.0f, fixedRandfs() * 150.0f},
      {fixedRandfs() * 150.0f, fixedRandfs() * 150.0f},
      {fixedRandfs() * 150.0f, fixedRandfs() * 150.0f}
    };
    for(auto &p : triPos) {
      p[0] += SCREEN_WIDTH / 2.0f;
      p[1] += SCREEN_HEIGHT / 2.0f;
    }

    RDP::DPL dplTri{64};
    dplTri.add(RDP::syncPipe())
      .add(RDP::setFillColor({0x22, 0x22, 0x22, 0}))
      .add(RDP::setScissor(TEST_REGION[0], TEST_REGION[1], TEST_REGION[2], TEST_REGION[3]))
      .add(RDP::fillRect(TEST_REGION[0], TEST_REGION[1], TEST_REGION[2], TEST_REGION[3]))
      .add(RDP::syncPipe())

      .add(RDP::setFillColorRaw(fixedRand()))
      .add(RDP::triangle(0,
        {.pos = {triPos[0][0], triPos[0][1]}},
        {.pos = {triPos[1][0], triPos[1][1]}},
        {.pos = {triPos[2][0], triPos[2][1]}}
       ))
      .runSync(TICKS_FROM_MS(100));

    // Load referende file
    const char* filePath = "rom:/00000000.test";
    sprintf((const_cast<char*>(filePath) + 5), "%08X.test", TEST_CASES[testIdx]);

    uint16_t *testDataOrg = nullptr;

    if(!(held.btn.z && held.btn.a)) {
      testDataOrg = (uint16_t *)asset_load(filePath, nullptr);
    }

    auto testData = testDataOrg ? testDataOrg : (uint16_t*)&TEST_CASES;

    bool isDump = pressed.b;
    if(isDump)debugf("TEST=%08X\n", TEST_CASES[testIdx]);

    // go pixel by pixel in the test region and compare
    // if pressed B, dump the framebuffer over debugf
    uint16_t* fbPtr = (uint16_t*)state.fb->buffer;
    fbPtr += TEST_REGION[1] * (state.fb->stride / 2);

    TEST_RES[testIdx] = 0;
    int totalPixel = 0;

    for(int y=TEST_REGION[1]; y<TEST_REGION[3]; ++y)
    {
      for(int x=TEST_REGION[0]; x<=TEST_REGION[2]; ++x)
      {
        uint16_t col = fbPtr[x];
        if(isDump)debugf("%04X", col);

        TEST_RES[testIdx] += (*testData != col) ? 1 : 0;
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
    Text::printf(px, py, "Errors:         (Test: %02d|%08X)", testIdx, TEST_CASES[testIdx]);
    py+=10;
    for(uint32_t t=0; t<TEST_CASES.size(); ++t)
    {
      if(TEST_RES[t] == 0xFFFF'FFFF) {
        Text::setColor({0x99, 0x99, 0x99});
        Text::printf(px, py, "---");
      } else {
        Text::setColor(
          TEST_RES[t] == 0 ? color_t{0x66, 0xFF, 0x66} : color_t{0xFF, 0x66, 0x66}
        );
        Text::printf(px, py, "%03X", TEST_RES[t]);
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
    if(testDoneCount == TEST_CASES.size()) {
      Text::printf(16, py, "Passed: %d/%d", testSuccessCount, TEST_CASES.size());
      px = 128;
      if(testSuccessCount == TEST_CASES.size()) {
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
}