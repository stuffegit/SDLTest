#include "gpu_context.hpp"

#include <iostream>

GPUContext::GPUContext(SDL_Window* window) {
  // SPIRV_ONLY tells SDL to use the Vulkan backend exclusively.
  // Vulkan is the only backend that speaks SPIR-V natively; passing this flag
  // avoids SDL trying to fall back to Direct3D or Metal with a shader it can't
  // use.
#ifdef NDEBUG
  constexpr bool kGPUDebug = false;
#else
  constexpr bool kGPUDebug = true;
#endif
  m_device =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, kGPUDebug, nullptr);
  if (!m_device) {
    std::cerr << "SDL_CreateGPUDevice failed: " << SDL_GetError() << '\n';
    return;
  }

  // "Claiming" the window hands it over to the GPU device for rendering.
  // After this call SDL knows which device owns the window's swapchain, so
  // SDL_AcquireGPUSwapchainTexture can hand us a render target each frame.
  if (!SDL_ClaimWindowForGPUDevice(m_device, window)) {
    std::cerr << "SDL_ClaimWindowForGPUDevice failed: " << SDL_GetError()
              << '\n';
    SDL_DestroyGPUDevice(m_device);
    m_device = nullptr;
    return;
  }

  // VSYNC ties frame presentation to the monitor's refresh rate (typically 60
  // or 144 Hz). It prevents screen tearing by only swapping the image on a
  // vertical blank. Note: SDL_SubmitGPUCommandBuffer returns immediately (GPU
  // work is async), so a separate CPU-side frame limiter is still needed to
  // avoid spinning at 9000+ FPS.
  if (!SDL_SetGPUSwapchainParameters(m_device, window,
                                     SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                     SDL_GPU_PRESENTMODE_VSYNC)) {
    std::cerr << "VSync not supported, running uncapped\n";
  }
}

GPUContext::~GPUContext() {
  if (m_device) {
    SDL_DestroyGPUDevice(m_device);
  }
}

bool GPUContext::isValid() const {
  return m_device != nullptr;
}
