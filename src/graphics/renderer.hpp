#pragma once

#include "mesh.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>

struct RenderObject {
  const Mesh* mesh;
  glm::mat4 modelView;
  glm::mat4 proj;
  glm::vec4 color;
};

struct FogParams {
  float strength = 1.0f;
  float depthStart = 9.0f;
  float depthEnd = 28.0f;
  float sideStart = 5.0f;
  float sideEnd = 10.0f;
};

class Renderer {
  SDL_GPUDevice* m_device = nullptr;
  SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
  SDL_GPUTexture* m_depthTexture = nullptr;
  FogParams m_fog;

public:
  explicit Renderer(SDL_GPUDevice* device);
  ~Renderer();

  [[nodiscard]] bool init(SDL_Window* window);
  void setFog(const FogParams& fog) {
    m_fog = fog;
  }

  void render(SDL_GPUCommandBuffer* cmdBuf, SDL_GPUTexture* swapchainTexture,
              const std::vector<RenderObject>& objects);
};
