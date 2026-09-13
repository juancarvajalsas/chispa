#include "../engine/engine.h"
#include <GLFW/glfw3.h>
#include <chrono>

const int GRID_W = 10;
const int GRID_H = 20;
int grid[GRID_H][GRID_W];
int blocksTex;
float fallTimer = 0.0f;
float fallInterval = 0.5f;

struct CellRect {
  float x, y, hw, hh;
};

struct Piece {
  int cells[4][2];
  int color;
  int x, y;
};

CellRect cellToScreen(int col, int row) {
  float cellSize = 1.9f / GRID_H;
  float halfCell = cellSize * 0.5f;

  float gridW = cellSize * GRID_W;
  float gridH = cellSize * GRID_H;

  float startX = -gridW * 0.5f + halfCell;
  float startY = -gridH * 0.5f + halfCell;

  CellRect r;
  r.x = startX + col * cellSize;
  r.y = startY + row * cellSize;
  r.hw = halfCell;
  r.hh = halfCell;
  return r;
}

void clearGrid() {
  for (int y = 0; y < GRID_H; y++)
    for (int x = 0; x < GRID_W; x++)
      grid[y][x] = -1;
}

void drawGrid() {
  for (int row = 0; row < GRID_H; row++) {
    for (int col = 0; col < GRID_W; col++) {
      CellRect c = cellToScreen(col, row);
      int cell = grid[row][col];
      if (cell >= 0) {
        float uvX = cell / 7.0f;
        drawSprite(blocksTex, c.x, c.y, c.hw, c.hh, uvX, 0.0f, 1.0f / 7.f,
                   1.0f);
      } else {
      }
    }
  }
}

Piece makePiece(int type) {
  Piece p;
  p.color = type;
  p.x = GRID_W / 2 - 1;
  p.y = 0;

  switch (type) {
  case 0: // I
    p.cells[0][0] = 0;
    p.cells[0][1] = 0;
    p.cells[1][0] = 1;
    p.cells[1][1] = 0;
    p.cells[2][0] = 2;
    p.cells[2][1] = 0;
    p.cells[3][0] = 3;
    p.cells[3][1] = 0;
    break;
  case 1: // O (square)
    p.cells[0][0] = 0;
    p.cells[0][1] = 0;
    p.cells[1][0] = 1;
    p.cells[1][1] = 0;
    p.cells[2][0] = 0;
    p.cells[2][1] = 1;
    p.cells[3][0] = 1;
    p.cells[3][1] = 1;
    break;
  case 2: // T
    p.cells[0][0] = 0;
    p.cells[0][1] = 0;
    p.cells[1][0] = 1;
    p.cells[1][1] = 0;
    p.cells[2][0] = 2;
    p.cells[2][1] = 0;
    p.cells[3][0] = 1;
    p.cells[3][1] = 1;
    break;
  case 3: // S
    p.cells[0][0] = 1;
    p.cells[0][1] = 0;
    p.cells[1][0] = 2;
    p.cells[1][1] = 0;
    p.cells[2][0] = 0;
    p.cells[2][1] = 1;
    p.cells[3][0] = 1;
    p.cells[3][1] = 1;
    break;
  case 4: // Z
    p.cells[0][0] = 0;
    p.cells[0][1] = 0;
    p.cells[1][0] = 1;
    p.cells[1][1] = 0;
    p.cells[2][0] = 1;
    p.cells[2][1] = 1;
    p.cells[3][0] = 2;
    p.cells[3][1] = 1;
    break;
  case 5: // J
    p.cells[0][0] = 0;
    p.cells[0][1] = 0;
    p.cells[1][0] = 0;
    p.cells[1][1] = 1;
    p.cells[2][0] = 1;
    p.cells[2][1] = 1;
    p.cells[3][0] = 2;
    p.cells[3][1] = 1;
    break;
  case 6: // L
    p.cells[0][0] = 2;
    p.cells[0][1] = 0;
    p.cells[1][0] = 0;
    p.cells[1][1] = 1;
    p.cells[2][0] = 1;
    p.cells[2][1] = 1;
    p.cells[3][0] = 2;
    p.cells[3][1] = 1;
    break;
  }
  return p;
}

void drawPiece(const Piece &p) {
  for (int i = 0; i < 4; i++) {
    int col = p.x + p.cells[i][0];
    int row = p.y + p.cells[i][1];
    if (row >= 0 && row < GRID_H && col >= 0 && col < GRID_W) {
      CellRect c = cellToScreen(col, row);
      float uvX = p.color / 7.0f;
      drawSprite(blocksTex, c.x, c.y, c.hw, c.hh, uvX, 0.0f, 1.0f / 7.0f, 1.0f);
    }
  }
}

bool canPlacePiece(const Piece &p) {
  for (int i = 0; i < 4; i++) {
    int col = p.x + p.cells[i][0];
    int row = p.y + p.cells[i][1];
    if (col < 0 || col >= GRID_W || row < 0 || row >= GRID_H)
      return false;
    if (grid[row][col] >= 0)
      return false;
  }
  return true;
}

bool canPlace(const Piece &p, int px, int py) {
  for (int i = 0; i < 4; i++) {
    int col = px + p.cells[i][0];
    int row = py + p.cells[i][1];
    if (col < 0 || col >= GRID_W || row < 0 || row >= GRID_H)
      return false;
    if (grid[row][col] >= 0)
      return false;
  }
  return true;
}

Piece rotated(const Piece &p) {
  Piece r = p;
  for (int i = 0; i < 4; i++) {
    int x = p.cells[i][0];
    int y = p.cells[i][1];

    r.cells[i][0] = -y;
    r.cells[i][1] = x;
  }
  return r;
}

void lockPiece(const Piece &p) {
  for (int i = 0; i < 4; i++) {
    int col = p.x + p.cells[i][0];
    int row = p.y + p.cells[i][1];
    if (row >= 0 && row < GRID_H && col >= 0 && col < GRID_W)
      grid[row][col] = p.color;
  }
}

int main() {
  engineInit(600, 800, "Tetris");
  blocksTex = loadTexture("tetris/textures/blocks.png", Filter::Nearest);
  clearGrid();

  Piece current = makePiece(rand() % 7);
  auto lastTime = std::chrono::high_resolution_clock::now();

  while (!engineShouldClose()) {
    enginePollEvents();
    if (isKeyDown(GLFW_KEY_ESCAPE)) {
      break;
    }

    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    if (isKeyPressed(GLFW_KEY_LEFT) &&
        canPlace(current, current.x - 1, current.y))
      current.x -= 1;
    if (isKeyPressed(GLFW_KEY_RIGHT) &&
        canPlace(current, current.x + 1, current.y))
      current.x += 1;
    if (isKeyPressed(GLFW_KEY_UP) && current.color != 1) {
      Piece r = rotated(current);
      if (canPlacePiece(r)) {
        current = r;
      }
    }

    float interval = isKeyDown(GLFW_KEY_DOWN) ? 0.05f : fallInterval;

    fallTimer += dt;
    if (fallTimer >= interval) {
      fallTimer = 0.0f;
      if (canPlace(current, current.x, current.y + 1)) {
        current.y += 1;
      } else {
        lockPiece(current);
        current = makePiece(rand() % 7);
      }
    }

    engineBeginFrame();
    drawGrid();
    drawPiece(current);
    engineEndFrame();
  }
  engineShutdown();
  return 0;
}
