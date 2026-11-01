#include "../engine/engine.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>

const float TURN_SPEED = 4.0f;
const float THRUST = 1.5f;
const float MAX_SPEED = 1.2f;
const float DRAG = 0.6f;
const float SHIP_SIZE = 0.06f;

struct Ship {
  float x = 0.0f, y = 0.0f;
  float vx = 0.0f, vy = 0.0f;
  float angle = 0.0f;
};

int main() {
  engineInit(800, 800, "Asteroid");
  int shipTex = loadTexture("asteroids/textures/ship.png", Filter::Linear);

  Ship ship;
  float time = 0.0f;
  auto lastTime = std::chrono::high_resolution_clock::now();

  while (!engineShouldClose()) {
    enginePollEvents();
    if (isKeyDown(GLFW_KEY_ESCAPE))
      break;

    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    dt = std::min(dt, 0.05f);
    time += dt;

    if (isKeyDown(GLFW_KEY_LEFT))
      ship.angle -= TURN_SPEED * dt;
    if (isKeyDown(GLFW_KEY_RIGHT))
      ship.angle += TURN_SPEED * dt;

    float fx = std::sin(ship.angle);
    float fy = -std::cos(ship.angle);

    bool thrusting = isKeyDown(GLFW_KEY_UP);
    if (thrusting) {
      ship.vx += fx * THRUST * dt;
      ship.vy += fy * THRUST * dt;
    }

    float damp = std::pow(DRAG, dt);
    ship.vx *= damp;
    ship.vy *= damp;

    float speed = std::sqrt(ship.vx * ship.vx + ship.vy * ship.vy);
    if (speed > MAX_SPEED) {
      ship.vx = ship.vx / speed * MAX_SPEED;
      ship.vy = ship.vy / speed * MAX_SPEED;
    }

    ship.x += ship.vx * dt;
    ship.y += ship.vy * dt;

    if (isKeyPressed(GLFW_KEY_R))
      ship = Ship{};

    engineBeginFrame();
    if (thrusting && std::fmod(time, 0.1f) < 0.05f) {
      float tailX = ship.x - fx * SHIP_SIZE * 1.1f;
      float tailY = ship.y - fy * SHIP_SIZE * 1.1f;
      drawQuad(tailX, tailY, 0.008f, 0.015f, ship.angle);
    }
    drawSprite(shipTex, ship.x, ship.y, SHIP_SIZE, SHIP_SIZE, 0, 0, 1, 1,
               ship.angle);

    engineEndFrame();
  }
  engineShutdown();
  return 0;
}
