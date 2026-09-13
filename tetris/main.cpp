#include "../engine/engine.h"
#include <GLFW/glfw3.h>
#include <cstdio>

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

  switch(type) {
    case 0: // I
    case 1: // O (square)
    case 2: // T
    case 3: // S
    case 4: // Z
    case 5: // J
    case 6: // L
  }
  return p;
}

int main() {
  engineInit(600, 800, "Tetris");
  blocksTex = loadTexture("tetris/textures/blocks.png", Filter::Nearest);

  clearGrid();
  for (int col = 0; col < GRID_W; col++) {
    grid[GRID_H - 1][col] = col % 7;
  }
  grid[0][0] = 0;
  grid[5][5] = 3;
  grid[10][2] = 4;

  while (!engineShouldClose()) {
    enginePollEvents();
    if (isKeyDown(GLFW_KEY_ESCAPE)) {
      break;
    }

    engineBeginFrame();
    drawGrid();
    engineEndFrame();
  }
  engineShutdown();
  return 0;
}
