/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include <cmath>
#include "../main.h"
#include "../rdp/rdp.h"
#include "../rdp/dpl.h"
#include "../text.h"

namespace {
  constexpr uint32_t posToScanline(float height) {
    return 36 + height * 2;
  }

  constexpr int PADDLE_POS_Y[2]{32, 200};
  constexpr int PADDLE_HEIGHT = 10;
  constexpr int PADDLE_WIDTH = 64;

  constexpr int BALL_START_Y = PADDLE_POS_Y[0] + PADDLE_HEIGHT;
  constexpr int BALL_SIZE = 16;

  constexpr uint32_t linePaddle0Start = posToScanline(PADDLE_POS_Y[0]);
  constexpr uint32_t linePaddle0End = posToScanline(PADDLE_POS_Y[0] + PADDLE_HEIGHT);
  constexpr uint32_t lineBallEnd = posToScanline(PADDLE_POS_Y[1]);

  constinit int paddlePosX[2]{32, 32};
  constinit float ballPos[2]{};

  constinit float ballVel[2]{1.04f, 2.2f};
  constinit float ballVelStart[2]{0.0f, 0.0f};

  constinit int points[2]{0,0};

  constinit uint32_t scanlineBall = 0;
  constinit uint32_t scanlineBallEnd = 0;

  bool ballVsPaddle(int idx) {
    // check ball position vs paddle
    if((ballPos[0]+BALL_SIZE) > paddlePosX[idx] && ballPos[0] < (paddlePosX[idx] + PADDLE_WIDTH)) {
      return true;
    }
    return false;
  }

  void respawn()
  {
    ballPos[0] = 16 + (rand() % 270);
    ballPos[1] = 120;
    ballVel[0] = 0.5f + (rand() % 16) / 16.0f;
    ballVel[1] = 0.5f + (rand() % 16) / 16.0f;
    if(rand() & 1)ballVel[0] = -ballVel[0];
    if(rand() & 1)ballVel[1] = -ballVel[1];

    ballVelStart[0] = 0;
    ballVelStart[1] = 0;
  }

  void reflect(int axis) {
    ballVel[0] *= 1.1f;
    ballVel[1] *= 1.1f;

    ballVel[axis] = -ballVel[axis];
    ballVel[1-axis] += (float)(rand() % 16) / 64.0f;
  }

  void updateGame()
  {
    constexpr int MAX_PADDLE_X = 240;

    // input
    auto held = joypad_get_buttons_held(JOYPAD_PORT_1);
    if(held.c_left || held.d_left)paddlePosX[0] -= 2;
    if(held.c_right || held.d_right)paddlePosX[0] += 2;

    // AI
    if(ballPos[1] > 70) {
      int speed = ballPos[1] > 90 ? 2 : 1;
      int ballMidX = ballPos[0] + BALL_SIZE/2;
      int paddleMidX = paddlePosX[1] + PADDLE_WIDTH/2;
      if(ballMidX < paddleMidX) {
        paddlePosX[1] -= speed;
      } else if(ballMidX > paddleMidX) {
        paddlePosX[1] += speed;
      }
    }

    for(int i=0; i<2; ++i) {
      if(paddlePosX[i] < 0)paddlePosX[i] = 0;
      if(paddlePosX[i] > MAX_PADDLE_X)paddlePosX[i] = MAX_PADDLE_X;
    }

    // ball
    for(int i=0; i<2; ++i) {
      ballVelStart[i] += 0.05f;
      if(ballVelStart[i] > 1.0f)ballVelStart[i] = 1.0f;

      ballPos[i] += ballVel[i] * (ballVelStart[i] * ballVelStart[i]);
    }


    // reflect on screen edges
    if(ballPos[0] < 0) {
      ballPos[0] = 0;
      reflect(0);
    }

    if(ballPos[0]+BALL_SIZE > MAX_PADDLE_X+PADDLE_WIDTH) {
      ballPos[0] = MAX_PADDLE_X+PADDLE_WIDTH-BALL_SIZE;
      reflect(0);
    }

    if(ballPos[1] < (PADDLE_POS_Y[0] + PADDLE_HEIGHT)) {
      if(ballVsPaddle(0)) {
        ballPos[1] = PADDLE_POS_Y[0] + PADDLE_HEIGHT;
        reflect(1);
      } else {
        ++points[1];
        respawn();
      }
    }

    if((ballPos[1] + BALL_SIZE) > PADDLE_POS_Y[1]) {
      if(ballVsPaddle(1)) {
        ballPos[1] = PADDLE_POS_Y[1] - BALL_SIZE;
        reflect(1);
      } else {
        ++points[0];
        respawn();
      }
    }

    scanlineBall = posToScanline(ballPos[1]);
    scanlineBallEnd = posToScanline(ballPos[1] + 14);
  }
}

namespace Demo::VIPong
{
  extern const char* const name = "VI Pong";

  void init()
  {
    paddlePosX[0] = 0;
    paddlePosX[1] = 0;
    ballPos[0] = 0;
    ballPos[1] = 0;
    points[0] = 0;
    points[1] = 0;
    respawn();
  }

  void destroy() {

  }

  void draw()
  {
    RDP::DPL dpl{64};
    dpl.add(RDP::syncPipe())
      .add(RDP::setColorImage(state.fb->buffer, RDP::Format::RGBA, RDP::BBP::_16, state.fb->stride/2))
      .add(RDP::setScissor(0, 0, state.fb->width-1, state.fb->height-1))
      .add(RDP::setOtherModes(RDP::OtherMode()
        .cycleType(RDP::CYCLE::FILL)
      ))
      // 2 paddles
      .add(RDP::syncPipe())
      .add(RDP::setFillColor({0xAA, 0xFF, 0xAA, 0xFF}))
      .add(RDP::fillRect(0,PADDLE_POS_Y[0], PADDLE_WIDTH, PADDLE_POS_Y[0] + PADDLE_HEIGHT))
      .add(RDP::syncPipe())
      .add(RDP::setFillColor({0x11, 0x11, 0x11, 0xFF}))
      .add(RDP::fillRect(5, PADDLE_POS_Y[0]+2, PADDLE_WIDTH-2, PADDLE_POS_Y[0] + PADDLE_HEIGHT-2))

      .add(RDP::syncPipe())
      .add(RDP::setFillColor({0xFF, 0xAA, 0xAA, 0xFF}))
      .add(RDP::fillRect(0,PADDLE_POS_Y[1], PADDLE_WIDTH, PADDLE_POS_Y[1] + PADDLE_HEIGHT))
      .add(RDP::syncPipe())
      .add(RDP::setFillColor({0x11, 0x11, 0x11, 0xFF}))
      .add(RDP::fillRect(5, PADDLE_POS_Y[1]+2, PADDLE_WIDTH-2, PADDLE_POS_Y[1] + PADDLE_HEIGHT-2))

      // ball, stretches entire height
      .add(RDP::syncPipe())
      .add(RDP::setFillColor({0x66, 0x66, 0xFF, 0xFF}))
      .add(RDP::fillRect(0, BALL_START_Y + 2, 16, PADDLE_POS_Y[1] - 2))
      .runAsync();

    updateGame();

    uint32_t orgHVideo = *VI_H_VIDEO;
    uint32_t lastLine = *VI_V_CURRENT;

    if(state.frame < 3)return;

    //vi_debug_dump(1);

    constexpr int startLine = 20;

    while(lastLine < 480) {
      uint32_t scanLine = *VI_V_CURRENT;
      if(scanLine != lastLine) {
        if(lastLine > startLine) {

          int pixelStart = 108;
          int pixelEnd = 748;

          // paddle 0
          if(scanLine >= linePaddle0Start && scanLine <= linePaddle0End) {
            pixelStart += paddlePosX[0] * 2;
          }

          // paddle 1
          if(scanLine > lineBallEnd) {
            pixelStart += paddlePosX[1] * 2;
          } else if(scanLine > linePaddle0End) {
            // ball
            if(scanLine >= scanlineBall && scanLine <= scanlineBallEnd) {
              pixelStart += (int)ballPos[0]*2;
            } else {
              pixelStart = 0;
              pixelEnd = 0;
            }
          }

          MEMORY_BARRIER();
          *VI_H_VIDEO = pixelEnd | (pixelStart << 16);
          MEMORY_BARRIER();

        }
        lastLine = scanLine;
      }
    }

    MEMORY_BARRIER();
    *VI_H_VIDEO = orgHVideo;
    MEMORY_BARRIER();

    Text::print(16, 240-16, "[VI-Pong]");

    Text::printf(140, 240-16, "Points: %d ~ %d", points[0], points[1]);
  }
}