#pragma once

#include "debug_overlay.hpp"
#include "gpu_context.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include <SDL3/SDL.h>
#include <memory>

/**
 * @brief Top-level application object.
 *
 * Owns all subsystems (window, GPU context, renderer, meshes) and drives
 * the main loop.
 * Exactly one instance should exist for the lifetime of the
 * process.
 * Destruction order of members matters — see shutdown().
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
  App();
  ~App();

  /**
   * @brief Initializes all subsystems then runs the main loop until quit.
   *
   * @return 0 on clean exit, 1 on init failure.
   */
  int run();
};
