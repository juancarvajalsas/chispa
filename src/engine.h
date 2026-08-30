#pragma once

// --- LifeCicle ---
void engineInit(int width, int height, const char *title);
void engineShutdown();
bool engineShouldClose();
void enginePollEvents();

// --- Frame ---
void engineBeginFrame();
void engineEndFrame();

// --- Drawing ---
void drawQuad(float x, float y, float halfW, float halfH);
void drawSprite(float x, float y, float halfW, float halfH, float uvX,
                float uvY, float uvW, float uvH);

// --- Input ---
bool isKeyDown(int key);
bool isKeyPressed(int key);
