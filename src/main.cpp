/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <libdragon.h>
#include <vector>
#include "text.h"
#include "main.h"

#define DEMO_ENTRY(X) namespace Demo::X { \
  void init(); void draw(); void destroy(); extern const char* const name; \
}

#include "demoList.h"
#undef DEMO_ENTRY

namespace {
  /*void testText()
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
  }*/

  typedef void (*DemoFunc)();

  struct DemoEntry
  {
    DemoFunc init{};
    DemoFunc draw{};
    DemoFunc destroy{};
    const char* name{};
  };

  constinit uint64_t frameTime = 0;
  constinit uint32_t currDemo = 0xFFFF;
  constinit uint32_t nextDemo = 1;

  volatile int freeFB = 3;
  void on_vi_frame_ready()
  {
    disable_interrupts();
    if(freeFB < 3) {
      freeFB += 1;
    }
    enable_interrupts();
  }

  std::vector<DemoEntry> demos{};
  uint32_t nextDemoSel = 1;

  void demoMenuDraw()
  {
    memset(state.fb->buffer, 0, state.fb->height * state.fb->stride);

    auto press = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if(press.d_up || press.c_up)--nextDemoSel;
    if(press.d_down || press.c_down)++nextDemoSel;
    if(nextDemoSel == 0)nextDemoSel = demos.size()-1;
    if(nextDemoSel >= demos.size())nextDemoSel = 1;

    if(press.a || press.b) {
      nextDemo = nextDemoSel;
    }

    constexpr color_t colSel{0x66, 0x66, 0xFF};

    for(uint32_t i = 1; i < demos.size(); ++i) {
      Text::setColor(i == nextDemoSel ? colSel : color_t{0xFF, 0xFF, 0xFF});
      Text::print(40, 26 + i * 10, demos[i].name);
    }

    Text::setColor(colSel);
    Text::print(28, 26 + nextDemoSel * 10 - 1, ">");
    Text::setColor();

    int posY = 160;
    Text::print(20, posY, "UP/DOWN/A - Select"); posY += 10;
    Text::print(20, posY, "Start     - Open this Menu"); posY += 10;
    Text::print(20, posY, "L/R       - Toggle Demo"); posY += 10;

    posY += 10;

    Text::setColor({0x66, 0xFF, 0x33});
    Text::print(20, posY, "Repo:"); posY += 10;
    Text::print(20, posY, "github.com/HailToDodongo/repeater64"); posY += 10;
    Text::setColor();
  }
}

constinit State state{};

[[noreturn]]
int main()
{
  #define DEMO_ENTRY(X) {Demo::X::init, Demo::X::draw, Demo::X::destroy, Demo::X::name},
  demos = {
    DemoEntry{.draw = demoMenuDraw},
    #include "demoList.h"
  };
  #undef DEMO_ENTRY

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
    auto held = joypad_get_buttons_held(JOYPAD_PORT_1);
    auto press = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if(press.r){ nextDemo = (currDemo + 1) % demos.size(); if(nextDemo == 0)nextDemo = 1; }
    if(press.l){ nextDemo = (currDemo - 1) % demos.size(); if(nextDemo == 0)nextDemo = demos.size()-1; }
    if(press.start)nextDemo = 0;

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

      if(currDemo < demos.size() && demos[currDemo].destroy) {
        demos[currDemo].destroy();
      }

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

    frameTime = get_ticks() - t;

    vi_show(state.fb);
    //vi_wait_vblank();
  }
}
