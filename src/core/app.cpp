#include "app.hpp"
#include "window.hpp"
#include "gpu_context.hpp"
#include "renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

App::App() = default;
App::~App() { shutdown(); }

bool App::init() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return false;
  }

  m_window = std::make_unique<Window>("SDL3 GPU", 800, 600);
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
  m_mesh = Mesh::load(m_gpuContext->device(), basePath + "assets/meshes/cube.obj");
  if (!m_mesh) {
    std::cerr << "Mesh load failed\n";
    return false;
  }

  m_running = true;
  return true;
}

void App::handleEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT) {
      m_running = false;
    }
  }
}

void App::update(double dt) {
  m_angleX += static_cast<float>(dt) * 0.6f;
  m_angleY += static_cast<float>(dt) * 1.1f;
}

void App::render() {
  glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_angleX, glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, m_angleY, glm::vec3(0.0f, 1.0f, 0.0f));

  glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 4.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

  int w, h;
  SDL_GetWindowSizeInPixels(m_window->get(), &w, &h);
  float aspect = static_cast<float>(w) / static_cast<float>(h);
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
  proj[1][1] *= -1.0f;  // Vulkan NDC: Y points down

  glm::mat4 mvp = proj * view * model;

  SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(m_gpuContext->device());
  if (!cmdBuf) {
    return;
  }

  SDL_GPUTexture* swapchainTexture = nullptr;
  SDL_AcquireGPUSwapchainTexture(cmdBuf, m_window->get(), &swapchainTexture, nullptr, nullptr);

  if (swapchainTexture) {
    m_renderer->render(cmdBuf, swapchainTexture, *m_mesh, mvp);
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
    double dt = static_cast<double>(frameStart - last) / static_cast<double>(freq);
    last = frameStart;

    handleEvents();
    update(dt);
    render();

    m_debugInfo.frameTimeMs = static_cast<float>(dt * 1000.0);
    m_debugInfo.fps = (dt > 0.0) ? static_cast<float>(1.0 / dt) : 0.0f;
    m_debugOverlay.update(m_debugInfo);

    Uint64 elapsed = SDL_GetPerformanceCounter() - frameStart;
    Uint64 elapsedNS = elapsed * 1'000'000'000 / freq;
    if (elapsedNS < TARGET_NS) {
      SDL_DelayNS(TARGET_NS - elapsedNS);
    }
  }

  return 0;
}

void App::shutdown() {
  m_mesh.reset();
  m_renderer.reset();
  m_gpuContext.reset();
  m_window.reset();
  SDL_Quit();
}
