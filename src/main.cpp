/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <libdragon.h>
#include <utility>
#include "text.h"
#include "main.h"

// Demos
#include "demos/demoVI.h"
#include "demos/demoRepeat.h"
#include "demos/demoSync.h"
#include "demos/demoVIPong.h"

namespace {
  void testText()
  {
    int posY = 32;
    for(int i=0; i<4; ++i) {
      Text::print(16, posY, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"); posY += 8;
      Text::print(16, posY, "abcdefghijklmnopqrstuvwxyz"); posY += 8;
      Text::print(16, posY, "0123456789"); posY += 8;
      Text::print(16, posY, "!@#$%^&*()-_=+[]{};:"); posY += 8;
      Text::print(16, posY, "'\",.<>/?\\|`~"); posY += 8;
      posY += 8;
    }
  }

  typedef void (*DemoFunc)();

  struct DemoEntry
  {
    DemoFunc init{};
    DemoFunc draw{};
    DemoFunc destroy{};
  };

  constinit DemoEntry demos[] = {
    {}, // dummy entry
    {Demo::Sync::init,   Demo::Sync::draw,   Demo::Sync::destroy},
    {Demo::Repeat::init, Demo::Repeat::draw, nullptr},
    {Demo::VI::init,     Demo::VI::draw,     Demo::VI::destroy},
    {Demo::VIPong::init, Demo::VIPong::draw, Demo::VIPong::destroy},
    //{nullptr, testText, nullptr}
  };
  constexpr uint32_t DEMO_COUNT = sizeof(demos) / sizeof(DemoEntry);

  constinit uint64_t frameTime = 0;
  constinit uint32_t currDemo = 0;
  constinit uint32_t nextDemo = 1;

  constinit uint32_t infoTextCounter = 60*4;

  volatile int freeFB = 3;
  void on_vi_frame_ready()
  {
    disable_interrupts();
    if(freeFB < 3) {
      freeFB += 1;
    }
    enable_interrupts();
  }
}

constinit State state{};

[[noreturn]]
int main()
{
  debug_init_isviewer();
  debug_init_usblog();

  dfs_init(DFS_DEFAULT_LOCATION);

  joypad_init();

  vi_init();
  vi_set_dedither(false);
  vi_set_aa_mode(VI_AA_MODE_RESAMPLE);
  vi_set_interlaced(false);
  vi_set_divot(false);
  vi_set_gamma(VI_GAMMA_DISABLE);
  wait_ms(14);

  disable_interrupts();
    register_VI_handler(on_vi_frame_ready);
    set_VI_interrupt(1, VI_V_CURRENT_VBLANK);
  enable_interrupts();

  surface_t fbs[3] = {
    // Note: stride must be 0x800, since a single MI-repeat write will wrap within a 0x800 boundary
    surface_make((char*)0xA0300000, FMT_RGBA16, 320, 240, 0x800),
    surface_make((char*)0xA0380000, FMT_RGBA16, 320, 240, 0x800),
    surface_make((char*)0xA0400000, FMT_RGBA16, 320, 240, 0x800),
  };

  state.frame = 0;

  for(;;) 
  {
    ++state.frame;
    if(state.tripleBuffer) {
      state.fb = &fbs[state.frame % 3];
    } else {
      state.fb = &fbs[0];
    }

    joypad_poll();
    auto press = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if(press.r){ nextDemo = (currDemo + 1) % DEMO_COUNT; if(nextDemo == 0)nextDemo = 1; }
    if(press.l){ nextDemo = (currDemo - 1) % DEMO_COUNT; if(nextDemo == 0)nextDemo = DEMO_COUNT-1; }

    while(freeFB == 0) {
      vi_wait_vblank();
    }

    disable_interrupts();
    freeFB -= 1;
    enable_interrupts();

    uint64_t t = get_ticks();

    state.time += 0.025f;
    state.timeInt += 50;

    if(currDemo != nextDemo) {
      vi_wait_vblank();
      state.time = 0;
      state.timeInt = 0;
      state.tripleBuffer = true;
      state.showFrameTime = true;

      if(demos[currDemo].destroy)demos[currDemo].destroy();

      // clear all 3 framebuffers to black
      for(int i=0; i<3; ++i) {
        memset(fbs[i].buffer, 0, fbs[i].height * fbs[i].stride);
      }

      currDemo = nextDemo;
      if(demos[currDemo].init)demos[currDemo].init();
    }

    demos[currDemo].draw();

    if(state.showFrameTime) {
      Text::printf(16, 16, "%.2fms", TICKS_TO_US(frameTime) * (1.0f / 1000.0f));
    }

    if(infoTextCounter) {
      Text::print(80, 10 + infoTextCounter/6, "Press L/R To change Demo");
      --infoTextCounter;
    }

    frameTime = get_ticks() - t;

    vi_show(state.fb);
    //vi_wait_vblank();
  }
}
