#include "app.hpp"
#include "gpu_context.hpp"
#include "renderer.hpp"
#include "test_map.hpp"
#include "window.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

App::App() = default;
App::~App() {
  shutdown();
}

bool App::init() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return false;
  }

  m_window = std::make_unique<Window>("SDL3 GPU", 1920, 1080);
  if (!m_window->isValid()) {
    std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
    return false;
  }

  m_gpuContext = std::make_unique<GPUContext>(m_window->get());
  if (!m_gpuContext->isValid()) {
    return false;
  }

  m_renderer = std::make_unique<Renderer>(m_gpuContext->device());
  if (!m_renderer->init(m_window->get())) {
    std::cerr << "Renderer init failed\n";
    return false;
  }

  const char* base = SDL_GetBasePath();
  std::string basePath(base ? base : "");

  m_settingsPath = basePath + "settings.json";
  m_settings = loadSettings(m_settingsPath);

  m_scene = std::make_unique<TestMap>();
  if (!m_scene->init(m_gpuContext->device(), basePath)) {
    std::cerr << "Scene init failed\n";
    return false;
  }

  SDL_GPUTextureFormat swapchainFmt =
      SDL_GetGPUSwapchainTextureFormat(m_gpuContext->device(), m_window->get());
  if (!m_debugUI.init(m_window->get(), m_gpuContext->device(), swapchainFmt)) {
    std::cerr << "DebugUI init failed\n";
    return false;
  }

  m_running = true;
  return true;
}

void App::handleEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    m_debugUI.processEvent(e);
    if (e.type == SDL_EVENT_QUIT) {
      m_running = false;
    }
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
      m_running = false;
    }
  }
}

void App::update(double dt) {
  m_scene->configure(m_settings);
  m_scene->update(dt);
}

void App::render() {
  int w, h;
  SDL_GetWindowSizeInPixels(m_window->get(), &w, &h);
  float aspect = static_cast<float>(w) / static_cast<float>(h);

  std::vector<RenderObject> objects;
  glm::mat4 view, proj;
  m_scene->buildFrame(objects, view, proj, aspect);
  m_renderer->setFog(m_settings.fog);

  SDL_GPUCommandBuffer* cmdBuf =
      SDL_AcquireGPUCommandBuffer(m_gpuContext->device());
  if (!cmdBuf) {
    return;
  }

  SDL_GPUTexture* swapchainTexture = nullptr;
  SDL_AcquireGPUSwapchainTexture(cmdBuf, m_window->get(), &swapchainTexture,
                                 nullptr, nullptr);
  if (swapchainTexture) {
    m_renderer->render(cmdBuf, swapchainTexture, objects);
    m_debugUI.render(cmdBuf, swapchainTexture, m_settings, m_fps,
                     m_frameTimeMs);
  }

  SDL_SubmitGPUCommandBuffer(cmdBuf);
}

int App::run() {
  if (!init()) {
    return 1;
  }

  constexpr int TARGET_FPS = 144;
  constexpr Uint64 TARGET_NS = 1'000'000'000 / TARGET_FPS;

  Uint64 freq = SDL_GetPerformanceFrequency();
  Uint64 last = SDL_GetPerformanceCounter();

  while (m_running) {
    Uint64 frameStart = SDL_GetPerformanceCounter();
    double dt =
        static_cast<double>(frameStart - last) / static_cast<double>(freq);
    last = frameStart;

    m_fps = (dt > 0.0) ? static_cast<float>(1.0 / dt) : 0.0f;
    m_frameTimeMs = static_cast<float>(dt * 1000.0);

    handleEvents();
    update(dt);
    render();

    Uint64 elapsed = SDL_GetPerformanceCounter() - frameStart;
    Uint64 elapsedNS = elapsed * 1'000'000'000 / freq;
    if (elapsedNS < TARGET_NS) {
      SDL_DelayNS(TARGET_NS - elapsedNS);
    }
  }
  printf("\nExit called.\n");
  return 0;
}

void App::shutdown() {
  saveSettings(m_settings, m_settingsPath);
  SDL_WaitForGPUIdle(m_gpuContext ? m_gpuContext->device() : nullptr);
  m_debugUI.shutdown();
  m_scene.reset();
  m_renderer.reset();
  m_gpuContext.reset();
  m_window.reset();
  SDL_Quit();
}
