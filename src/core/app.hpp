#pragma once

#include "debug_ui.hpp"
#include "gpu_context.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "settings.hpp"
#include "window.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <string>

class App {
  bool m_running = false;
  std::unique_ptr<Window> m_window;
  std::unique_ptr<GPUContext> m_gpuContext;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<Scene> m_scene;
  DebugUI m_debugUI;
  Settings m_settings;
  std::string m_settingsPath;
  float m_fps = 0.0f;
  float m_frameTimeMs = 0.0f;

  bool init();
  void handleEvents();
  void update(double dt);
  void render();
  void shutdown();

public:
  App();
  ~App();
  int run();
};
