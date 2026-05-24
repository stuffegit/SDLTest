#pragma once

#include "settings.hpp"
#include <SDL3/SDL.h>

class DebugUI {
public:
  bool init(SDL_Window* window, SDL_GPUDevice* device,
            SDL_GPUTextureFormat swapchainFormat);
  void shutdown();
  void processEvent(const SDL_Event& e);
  void render(SDL_GPUCommandBuffer* cmdBuf, SDL_GPUTexture* swapchainTexture,
              Settings& settings, float fps, float frameTimeMs);
};
