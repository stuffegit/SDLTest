#pragma once

#include "debug_overlay.hpp"
#include "gpu_context.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include <SDL3/SDL.h>
#include <memory>

/**
 * @brief owns window, GPU context, renderer, meshes, and drives the main loop.
 */
class App {
  bool m_running = false;
  std::unique_ptr<Window> m_window;
  std::unique_ptr<GPUContext> m_gpuContext;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<Mesh> m_mesh;
  DebugOverlay m_debugOverlay;
  DebugInfo m_debugInfo;
  float m_angleX = 0.0f;
  float m_angleY = 0.0f;

  bool init();
  void handleEvents();
  void update(double dt);
  void render();
  void shutdown();

public:
  int width{};
  int height{};
  App();
  ~App();

  int run();
};
