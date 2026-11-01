#include "../engine/engine.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <random>

constexpr float TAU = 2.0f * std::numbers::pi_v<float>;

const int MAX_ROCKS = 32;
const int START_ROCKS = 4;
const float ROCK_RADIUS[3] = {0.15f, 0.08f, 0.04f};
const float ROCK_SPEED[3] = {0.15f, 0.25f, 0.40f};
const float HITBOX = 0.85f;

const float TURN_SPEED = 4.0f;
const float THRUST = 1.5f;
const float MAX_SPEED = 1.2f;
const float DRAG = 0.6f;
const float SHIP_SIZE = 0.06f;

const int MAX_BULLETS = 8;
const float BULLET_SPEED = 1.8f;
const float BULLET_LIFE = 0.9;
const float BULLET_SIZE = 0.008f;

struct Rock {
  float x = 0, y = 0;
  float vx = 0, vy = 0;
  float angle = 0, spin = 0;
  int size = 0;
  bool alive = false;
};

std::mt19937 rng{std::random_device{}()};

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

float randf(float lo, float hi) {
  std::uniform_real_distribution<float> d(lo, hi);
  return d(rng);
}

void spawnRock(Rock rocks[], float x, float y, int size) {
  for (int i = 0; i < MAX_ROCKS; i++) {
    if (rocks[i].alive)
      continue;
    float dir = randf(0.0f, TAU);
    float speed = ROCK_SPEED[size] * randf(0.7f, 1.3f);
    rocks[i] = Rock{x,
                    y,
                    std::cos(dir) * speed,
                    std::sin(dir) * speed,
                    randf(0.0f, TAU),
                    randf(-1.5f, 1.5f),
                    size,
                    true};
    return;
  }
}

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
  int rockTex = loadTexture("asteroids/textures/rock.png", Filter::Linear);

  Rock rocks[MAX_ROCKS];
  float halfW, halfH;
  getWorldHalfSize(halfW, halfH);
  for (int n = 0; n < START_ROCKS; n++) {
    float x, y;
    do {
      x = randf(-halfW, halfW);
      y = randf(-halfH, halfH);
    } while (x * x + y * y < 0.5f * 0.5f);
    spawnRock(rocks, x, y, 0);
  }

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

    for (Rock &r : rocks) {
      if (!r.alive)
        continue;
      r.x += r.vx * dt;
      r.y += r.vy * dt;
      r.angle += r.spin * dt;
      wrap(r.x, r.y, ROCK_RADIUS[r.size]);
    }

    for (Bullet &b : bullets) {
      if (!b.alive)
        continue;
      for (Rock &r : rocks) {
        if (!r.alive)
          continue;
        float dx = b.x - r.x, dy = b.y - r.y;
        float rad = ROCK_RADIUS[r.size] * HITBOX + BULLET_SIZE;
        if (dx * dx + dy * dy < rad * rad) {
          Rock dead = r;
          r.alive = false;
          b.alive = false;
          if (dead.size < 2) {
            spawnRock(rocks, dead.x, dead.y, dead.size + 1);
            spawnRock(rocks, dead.x, dead.y, dead.size + 1);
          }
          break;
        }
      }
    }

    for (const Rock &r : rocks) {
      if (!r.alive)
        continue;
      float dx = ship.x - r.x, dy = ship.y - r.y;
      float rad = ROCK_RADIUS[r.size] * HITBOX + SHIP_SIZE * 0.7f;
      if (dx * dx + dy * dy < rad * rad) {
        ship = Ship{};
        break;
      }
    }

    engineBeginFrame();

    for (const Rock &r : rocks) {
      if (!r.alive)
        continue;
      float s = ROCK_RADIUS[r.size];
      drawSprite(rockTex, r.x, r.y, s, s, 0, 0, 1, 1, r.angle);
    }

    for (const Bullet &b : bullets) {
      if (b.alive)
        drawQuad(b.x, b.y, BULLET_SIZE, BULLET_SIZE);
    }

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
