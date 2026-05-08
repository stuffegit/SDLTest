#pragma once

#include <SDL3/SDL.h>

/**
 * @brief Owns the SDL_GPUDevice and binds it to a window for rendering.
 *
 * Must outlive any Renderer or Mesh that holds a pointer to its device.
 */
class GPUContext {
  SDL_GPUDevice* m_device = nullptr;

public:
  /**
   * @brief Creates the GPU device and claims the window for GPU rendering.
   *
   * @param window Pass Window::get(). Fails if no Vulkan-capable GPU is found.
   */
  explicit GPUContext(SDL_Window* window);
  ~GPUContext();

  /** @brief Returns false if device creation or window claim failed. */
  [[nodiscard]] bool isValid() const;

  /**
   * @brief Non-owning device pointer.
   *
   * Valid for the lifetime of this GPUContext. Pass to Renderer and Mesh::load().
   */
  [[nodiscard]] SDL_GPUDevice* device() const { return m_device; }
};
