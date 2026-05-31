#include "debug_ui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

bool DebugUI::init(SDL_Window* window, SDL_GPUDevice* device,
                   SDL_GPUTextureFormat swapchainFormat) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
    return false;
  }

  ImGui_ImplSDLGPU3_InitInfo info = {};
  info.Device = device;
  info.ColorTargetFormat = swapchainFormat;
  info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
  info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
  return ImGui_ImplSDLGPU3_Init(&info);
}

void DebugUI::shutdown() {
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void DebugUI::processEvent(const SDL_Event& e) {
  ImGui_ImplSDL3_ProcessEvent(&e);
}

void DebugUI::render(SDL_GPUCommandBuffer* cmdBuf,
                     SDL_GPUTexture* swapchainTexture, Settings& settings,
                     float fps, float frameTimeMs) {
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Debug");
  ImGui::Text("%.0f FPS  %.2f ms", fps, frameTimeMs);
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Strength", &settings.fog.strength, 0.0f, 1.0f);
    ImGui::SliderFloat("Depth Start", &settings.fog.depthStart, 0.0f, 50.0f);
    ImGui::SliderFloat("Depth End", &settings.fog.depthEnd, 0.0f, 100.0f);
    ImGui::SliderFloat("Side Start", &settings.fog.sideStart, 0.0f, 20.0f);
    ImGui::SliderFloat("Side End", &settings.fog.sideEnd, 0.0f, 40.0f);
  }
  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Offset Y", &settings.cameraOffsetY, 1.0f, 20.0f);
    ImGui::SliderFloat("Offset Z", &settings.cameraOffsetZ, 1.0f, 20.0f);
  }
  if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Player Speed", &settings.playerSpeed, 0.5f, 20.0f);
  }
  if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat3("Direction", &settings.light.dir.x, 0.01f, -1.0f,
                          1.0f)) {
      float len = glm::length(settings.light.dir);
      if (len > 0.0f) {
        settings.light.dir /= len;
      }
    }
    ImGui::ColorEdit3("Light Color", &settings.light.color.r);
    ImGui::ColorEdit3("Ambient Color", &settings.light.ambientColor.r);
  }
  ImGui::End();

  ImGui::Render();
  ImDrawData* drawData = ImGui::GetDrawData();

  // Upload vertex/index buffers — must happen before the render pass
  ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmdBuf);

  SDL_GPUColorTargetInfo colorTarget = {};
  colorTarget.texture = swapchainTexture;
  colorTarget.load_op = SDL_GPU_LOADOP_LOAD; // overlay on top of scene
  colorTarget.store_op = SDL_GPU_STOREOP_STORE;

  SDL_GPURenderPass* pass =
      SDL_BeginGPURenderPass(cmdBuf, &colorTarget, 1, nullptr);
  ImGui_ImplSDLGPU3_RenderDrawData(drawData, cmdBuf, pass);
  SDL_EndGPURenderPass(pass);
}
