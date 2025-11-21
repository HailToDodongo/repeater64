/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <libdragon.h>
#include "main.h"
#include "text.h"
#include "miMemory.h"

namespace {
  constexpr uint32_t FMT_BUFF_SIZE = 128;

  constinit int fbStride = 0x800;
  constinit uint16_t currColor = 0xFFFF;
  constinit uint8_t ignoreChar = 0;

  #include "font.h"
  #include "font64.h"
}

void Text::setSpaceHidden(bool hidden)
{
  ignoreChar = hidden ? 0 : 0xFF;
}

void Text::setColor(color_t color)
{
  currColor = color_to_packed16(color);
}

int Text::print(int x, int y, const char *str) {
  auto fbBuff = (uint8_t*)state.fb->buffer;
  uint64_t *buffStart = (uint64_t*)&fbBuff[y * fbStride + x*2];
  uint64_t col = (uint64_t)currColor;

  while(*str)
  {
    uint8_t charCode = (uint8_t)*str - ' ';
    uint64_t charData = FONT_8x8_DATA[charCode];
    uint64_t *buff = buffStart;
    ++x;

    uint64_t val;
    if(charCode != ignoreChar)
    {
      for(int y=0; y<8; ++y) {
        for(int x=0; x<2; ++x) {
          val  = (charData & 0b0001) ? (col << 48) : 0;
          val |= (charData & 0b0010) ? (col << 32) : 0;
          val |= (charData & 0b0100) ? (col << 16) : 0;
          val |= (charData & 0b1000) ? (col <<  0) : 0;

          *buff = val;
          charData >>= 4;
          ++buff;
        }
        buff += (fbStride/8 - 2);
      }
      // draw extra black line below
      buff[0] = 0;
      buff[1] = 0;
    }

    buffStart += 2;
    ++str;
  }
  //uint64_t charData = FONT_8x8_DATA[0];

  return x;
}

int Text::printf(int x, int y, const char *fmt, ...) {
  char buffer[FMT_BUFF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, FMT_BUFF_SIZE, fmt, args);
  va_end(args);
  return print(x, y, buffer);
}

int Text::printLarge(int x, int y, const char *str, const TextFX &conf) {
  constexpr int CHAR_INCR = 40;
  int charIdx = 0;

  auto fbBuff = (uint8_t*)state.fb->buffer;

  while(*str)
  {
    uint8_t charCode = (uint8_t)*str - ' ';
    uint32_t dataStart = FONT_64_IDX[charCode];

    if(conf.posCB) {
      conf.posCB(x, y, charIdx++);
    }

    if(charCode == 0 || x < -64 || x > 320) {
      ++str;
      x += CHAR_INCR;
      continue;
    }

    uint16_t *buffStart = (uint16_t*)&fbBuff[y * fbStride];
    uint16_t *buff = (uint16_t*)&fbBuff[y * fbStride + x*2];

    uint8_t *sizeData = &FONT_64_DATA[dataStart];
    int totalSize = 0;
    bool isSolid = false;

    while(totalSize != (64*64)) {
      uint8_t size = *sizeData;
      totalSize += size;
      ++sizeData;

      if(isSolid) {
        if(buff < buffStart || size < (16/2)) {
          for(int s=0; s<size; ++s) {
            buff[s] = conf.color;
          }
        } else {
          if(conf.color == 0) {
            MiMem::zeroUnaligned((volatile char*)buff, size * 2);
          } else {
            MiMem::write(buff, conf.color, size * 2);
          }
        }
      }

      buff += size;
      isSolid = !isSolid;

      if((totalSize % 64) == 0) {
        buff += (fbStride/2 - 64);
        buffStart += (fbStride/2);
        isSolid = false;
      }
    }

    ++str;
    x += CHAR_INCR;
  }

  return x;
}

