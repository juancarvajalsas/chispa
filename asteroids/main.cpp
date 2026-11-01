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
const int MAX_BULLETS = 8;
const float BULLET_SPEED = 1.8f;
const float BULLET_LIFE = 0.9;
const float BULLET_SIZE = 0.008f;

struct Bullet {
  float x = 0, y = 0;
  float vx = 0, vy = 0;
  float life = 0;
  bool alive = false;
};

struct Ship {
  float x = 0.0f, y = 0.0f;
  float vx = 0.0f, vy = 0.0f;
  float angle = 0.0f;
};

void wrap(float &x, float &y, float radius) {
  float halfW, halfH;
  getWorldHalfSize(halfW, halfH);

  float limitX = halfW + radius;
  float limitY = halfH + radius;

  if (x > limitX)
    x -= 2.0f * limitX;
  if (x < -limitX)
    x += 2.0f * limitX;
  if (y > limitY)
    y -= 2.0f * limitY;
  if (y < -limitY)
    y += 2.0f * limitY;
}

int main() {
  engineInit(800, 800, "Asteroid");
  int shipTex = loadTexture("asteroids/textures/ship.png", Filter::Linear);

  Bullet bullets[MAX_BULLETS];
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
    wrap(ship.x, ship.y, SHIP_SIZE);

    if (isKeyPressed(GLFW_KEY_SPACE)) {
      for (Bullet &b : bullets) {
        if (!b.alive) {
          b.alive = true;
          b.life = BULLET_LIFE;
          b.x = ship.x + fx * SHIP_SIZE;
          b.y = ship.y + fy * SHIP_SIZE;
          b.vx = ship.vx + fx * BULLET_SPEED;
          b.vy = ship.vy + fy * BULLET_SPEED;
          break;
        }
      }
    }

    for (Bullet &b : bullets) {
      if (!b.alive)
        continue;
      b.life -= dt;
      if (b.life <= 0.0f) {
        b.alive = false;
        continue;
      }
      b.x += b.vx * dt;
      b.y += b.vy * dt;
      wrap(b.x, b.y, BULLET_SIZE);
    }

    engineBeginFrame();
    if (thrusting && std::fmod(time, 0.1f) < 0.05f) {
      float tailX = ship.x - fx * SHIP_SIZE * 1.1f;
      float tailY = ship.y - fy * SHIP_SIZE * 1.1f;
      drawQuad(tailX, tailY, 0.008f, 0.015f, ship.angle);
    }
    for (const Bullet &b : bullets) {
      if (b.alive)
        drawQuad(b.x, b.y, BULLET_SIZE, BULLET_SIZE);
    }
    drawSprite(shipTex, ship.x, ship.y, SHIP_SIZE, SHIP_SIZE, 0, 0, 1, 1,
               ship.angle);

    engineEndFrame();
  }
  engineShutdown();
  return 0;
}
