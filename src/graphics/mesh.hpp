#pragma once

#include <SDL3/SDL.h>
#include <cstddef>
#include <memory>
#include <string>

struct Vertex {
  float px, py, pz;
  float u, v;
  float nx, ny, nz;
};

class Mesh {
  SDL_GPUDevice* m_device = nullptr;
  SDL_GPUBuffer* m_vertexBuffer = nullptr;
  Uint32 m_vertexCount = 0;

  explicit Mesh(SDL_GPUDevice* device);

public:
  ~Mesh();

  [[nodiscard]] static std::unique_ptr<Mesh> load(SDL_GPUDevice* device,
                                                  const std::string& path);
  [[nodiscard]] static std::unique_ptr<Mesh>
  createPlane(SDL_GPUDevice* device, float halfW, float halfD);
  [[nodiscard]] bool isValid() const;
  void draw(SDL_GPURenderPass* pass) const;
};
