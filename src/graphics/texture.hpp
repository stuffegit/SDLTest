#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>

class Texture {
  SDL_GPUDevice* m_device = nullptr;
  SDL_GPUTexture* m_texture = nullptr;

  explicit Texture(SDL_GPUDevice* device);

public:
  ~Texture();
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  [[nodiscard]] static std::unique_ptr<Texture> load(SDL_GPUDevice* device,
                                                     const std::string& path);
  [[nodiscard]] static std::unique_ptr<Texture>
  createWhite(SDL_GPUDevice* device);

  SDL_GPUTexture* get() const {
    return m_texture;
  }
  bool isValid() const {
    return m_texture != nullptr;
  }
};
