/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <libdragon.h>
#include <array>
#include <functional>

class RDPDumpTest
{
  private:
    constexpr static int TEST_CASE_COUNT = 20;

  public:
    uint32_t testIdx = 0;
    bool autoMode = false;

    std::array<uint32_t, TEST_CASE_COUNT> testCases{};
    std::array<uint32_t, TEST_CASE_COUNT> testRes{};
    std::array<int, 4> testRegion{0,0,0,0};

    void reset()
    {
      testIdx = 0;
      autoMode = true;
      testRes.fill(0xFFFF'FFFF);
    }

    void run(std::function<void(uint32_t)> fn);
};
