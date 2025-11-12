/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once

namespace Text
{
  int print(int x, int y, const char* str);
  int printf(int x, int y, const char *fmt, ...);

  typedef void (*TextFXCb)(int &x, int &y, int idx);

  struct TextFX {
    uint64_t color{};
    TextFXCb posCB{};
  };

  int printLarge(int x, int y, const char* str, const TextFX &fx);
}
