#include "engine.h"
#include <GLFW/glfw3.h>

#include <chrono>
#include <cmath>
#include <cstdio>

struct GameObject {
  float x, y;
  float halfW, halfH;
};

int main() {
  engineInit(800, 600, "Pong");
  // loadTexture("textures/digits.png");

  GameObject leftPaddle = {-0.8, 0.0f, 0.05f, 0.25f};
  GameObject rightPaddle = {0.8, 0.0f, 0.05f, 0.25f};
  GameObject ball = {0.0f, 0.0f, 0.04f, 0.04f};
  int scoreLeft = 0;
  int scoreRight = 0;

  float ballVelX = 0.6f;
  float ballVelY = 0.4f;

  bool paused = false;

  auto overlaps = [](const GameObject &a, const GameObject &b) {
    return std::abs(a.x - b.x) < (a.halfW + b.halfW) &&
           std::abs(a.y - b.y) < (a.halfH + b.halfH);
  };

  auto drawDigits = [](int d, float x, float y, float hw, float hh) {
    drawSprite(x, y, hw, hh, d * 0.1f, 0.0f, 0.1f, 1.0f);
  };

  auto lastTime = std::chrono::high_resolution_clock::now();
  while (!engineShouldClose()) {
    enginePollEvents();

    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    if (isKeyPressed(GLFW_KEY_P))
      paused = !paused;
    if (isKeyDown(GLFW_KEY_ESCAPE))
      break;

    if (!paused) {
      float speed = 1.5f;
      if (isKeyDown(GLFW_KEY_W) == GLFW_PRESS)
        leftPaddle.y -= speed * dt;
      if (isKeyDown(GLFW_KEY_S) == GLFW_PRESS)
        leftPaddle.y += speed * dt;

      if (isKeyDown(GLFW_KEY_UP) == GLFW_PRESS)
        rightPaddle.y -= speed * dt;
      if (isKeyDown(GLFW_KEY_DOWN) == GLFW_PRESS)
        rightPaddle.y += speed * dt;

      auto clampY = [](float &y) {
        if (y < -0.8)
          y = -0.8f;
        if (y > 0.8f)
          y = 0.8f;
      };

      clampY(leftPaddle.y);
      clampY(rightPaddle.y);

      ball.x += ballVelX * dt;
      ball.y += ballVelY * dt;

      if (ball.y < -0.9f) {
        ball.y = -0.9f;
        ballVelY = -ballVelY;
      }
      if (ball.y > 0.9f) {
        ball.y = 0.9f;
        ballVelY = -ballVelY;
      }

      if (overlaps(ball, leftPaddle) && ballVelX < 0.0f)
        ballVelX = -ballVelX;
      if (overlaps(ball, rightPaddle) && ballVelX > 0.0f)
        ballVelX = -ballVelX;

      if (ball.x < -1.0f) {
        scoreRight++;
        ball.x = 0.0f;
        ball.y = 0.0f;
        ballVelX = 0.6f;
        ballVelY = 0.4f;
        printf("Score: %d - %d!\n", scoreLeft, scoreRight);
      }

      if (ball.x > 1.0f) {
        scoreLeft++;
        ball.x = 0.0f;
        ball.y = 0.0f;
        ballVelX = -0.6f;
        ballVelY = 0.4f;
        printf("Score: %d - %d!\n", scoreLeft, scoreRight);
      }
    }
    engineBeginFrame();
    drawQuad(leftPaddle.x, leftPaddle.y, leftPaddle.halfW, leftPaddle.halfH);
    drawQuad(rightPaddle.x, rightPaddle.y, rightPaddle.halfW, rightPaddle.halfH);
    drawQuad(ball.x, ball.y, ball.halfW, ball.halfH);
    drawDigits(scoreLeft, -0.3f, -0.8f, 0.05f, 0.1f);
    drawDigits(scoreRight, 0.3f, -0.8f, 0.05f, 0.1f);
    engineEndFrame();
  }

  engineShutdown();
  printf("Bye!");
  return 0;
}
