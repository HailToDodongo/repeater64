/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <libdragon.h>

namespace {
  constexpr uint32_t SCREEN_WIDTH = 320;
  constexpr uint32_t SCREEN_HEIGHT = 240;
}

struct State
{
  float time{};
  uint32_t timeInt{};
  surface_t *fb{};
  uint32_t frame{};
  bool tripleBuffer{true};
  bool showFrameTime{true};
};

extern State state;