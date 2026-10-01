#include "../engine/engine.h"
#include <GLFW/glfw3.h>
#include <chrono>

int main() {
  engineInit(800, 800, "Asteroid");
  int shipTex = loadTexture("asteroids/textures/ship.png", Filter::Linear);

  float angle = 0.0f;
  auto lastTime = std::chrono::high_resolution_clock::now();

  while (!engineShouldClose()) {
    enginePollEvents();
    if (isKeyDown(GLFW_KEY_ESCAPE))
      break;

    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    angle += 1.5f * dt;

    engineBeginFrame();
    drawQuad(0.0f, 0.0f, 0.15f, 0.15f);
    drawSprite(shipTex, 0.0f, 0.0f, 0.2f, 0.2f, 0, 0, 1, 1, angle);

    engineEndFrame();
  }
  engineShutdown();
  return 0;
}
