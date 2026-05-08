#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "mesh.hpp"

/**
 * @brief Owns the graphics pipeline and per-frame GPU resources (depth buffer).
 *
 * A pipeline is a compiled GPU object that encodes shader stages, vertex
 * layout, depth testing, and rasterizer settings all at once. It is created
 * once in init() and reused every frame. Switching pipelines mid-frame is
 * expensive, so a future engine would batch draw calls by pipeline.
 */
class Renderer {
  SDL_GPUDevice* m_device = nullptr;
  SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
  SDL_GPUTexture* m_depthTexture = nullptr;

public:
  /** @brief device is non-owning — must remain valid for the lifetime of this Renderer. */
  explicit Renderer(SDL_GPUDevice* device);
  ~Renderer();

  /**
   * @brief Compiles the graphics pipeline and creates the depth buffer.
   *
   * @param window Used to query the swapchain pixel format and window size.
   *               Must already be claimed by a GPUContext before calling this.
   * @return false on any GPU resource creation failure.
   */
  [[nodiscard]] bool init(SDL_Window* window);

  /**
   * @brief Records one frame: clear, draw mesh, end pass.
   *
   * @param cmdBuf           Active command buffer for this frame.
   * @param swapchainTexture Render target from SDL_AcquireGPUSwapchainTexture.
   * @param mesh             Geometry to draw — must be valid.
   * @param mvp              Model-view-projection matrix pushed to the vertex shader.
   */
  void render(SDL_GPUCommandBuffer* cmdBuf, SDL_GPUTexture* swapchainTexture,
              const Mesh& mesh, const glm::mat4& mvp);
};
