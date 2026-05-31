#pragma once

#include "mesh.hpp"
#include "texture.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct RenderObject {
  const Mesh* mesh;
  glm::mat4 modelView;
  glm::mat4 proj;
  glm::vec4 color;
  float shininess = 32.0f;
  float specStrength = 0.3f;
  const Texture* texture = nullptr; // null = white fallback
};

struct FogParams {
  float strength = 1.0f;
  float depthStart = 9.0f;
  float depthEnd = 28.0f;
  float sideStart = 5.0f;
  float sideEnd = 10.0f;
};

struct LightParams {
  glm::vec3 dir = glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f));
  glm::vec3 color = {1.0f, 0.95f, 0.8f};
  glm::vec3 ambientColor = {0.2f, 0.25f, 0.35f};
};

class Renderer {
  SDL_GPUDevice* m_device = nullptr;
  SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
  SDL_GPUTexture* m_depthTexture = nullptr;
  SDL_GPUSampler* m_sampler = nullptr;
  std::unique_ptr<Texture> m_whiteTexture;
  FogParams m_fog;
  LightParams m_light;

public:
  explicit Renderer(SDL_GPUDevice* device);
  ~Renderer();

  [[nodiscard]] bool init(SDL_Window* window, const std::string& basePath);
  void setFog(const FogParams& fog) {
    m_fog = fog;
  }
  void setLight(const LightParams& l) {
    m_light = l;
  }

  void render(SDL_GPUCommandBuffer* cmdBuf, SDL_GPUTexture* swapchainTexture,
              const std::vector<RenderObject>& objects,
              const glm::mat4& viewMatrix);
};
