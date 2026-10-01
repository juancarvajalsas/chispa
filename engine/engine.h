#pragma once

// --- LifeCicle ---
void engineInit(int width, int height, const char *title);
void engineShutdown();
bool engineShouldClose();
void enginePollEvents();

// --- Frame ---
void engineBeginFrame();
void engineEndFrame();

// --- Textures ---
enum class Filter { Nearest, Linear };
int loadTexture(const char *path, Filter filter = Filter::Linear);
// --- Drawing ---
void drawQuad(float x, float y, float halfW, float halfH, float angle = 0.0f);
void drawSprite(int handle, float x, float y, float halfW, float halfH,
                float uvX, float uvY, float uvW, float uvH, float angle = 0.0f);

// --- Input ---
bool isKeyDown(int key);
bool isKeyPressed(int key);
