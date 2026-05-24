#include "renderer.hpp"

#include <fstream>
#include <iostream>
#include <vector>

static std::vector<Uint8> loadSPV(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    std::cerr << "Failed to open shader: " << path << '\n';
    return {};
  }
  auto size = static_cast<std::streamsize>(file.tellg());
  file.seekg(0);
  std::vector<Uint8> data(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(data.data()), size);
  return data;
}

static SDL_GPUShader* createShader(SDL_GPUDevice* device,
                                   const std::string& path,
                                   SDL_GPUShaderStage stage,
                                   Uint32 numUniformBuffers) {
  auto code = loadSPV(path);
  if (code.empty()) {
    return nullptr;
  }

  SDL_GPUShaderCreateInfo info = {};
  info.code = code.data();
  info.code_size = code.size();
  info.entrypoint = "main";
  info.format = SDL_GPU_SHADERFORMAT_SPIRV;
  info.stage = stage;
  info.num_uniform_buffers = numUniformBuffers;

  SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
  if (!shader) {
    std::cerr << "SDL_CreateGPUShader failed: " << SDL_GetError() << '\n';
  }
  return shader;
}

Renderer::Renderer(SDL_GPUDevice* device) : m_device(device) {
}

Renderer::~Renderer() {
  if (m_depthTexture) {
    SDL_ReleaseGPUTexture(m_device, m_depthTexture);
  }
  if (m_pipeline) {
    SDL_ReleaseGPUGraphicsPipeline(m_device, m_pipeline);
  }
}

bool Renderer::init(SDL_Window* window) {
  const char* base = SDL_GetBasePath();
  std::string basePath(base ? base : "");

  SDL_GPUShader* vert =
      createShader(m_device, basePath + "assets/shaders/mesh.vert.spv",
                   SDL_GPU_SHADERSTAGE_VERTEX, 1);
  SDL_GPUShader* frag =
      createShader(m_device, basePath + "assets/shaders/mesh.frag.spv",
                   SDL_GPU_SHADERSTAGE_FRAGMENT, 1);

  if (!vert || !frag) {
    SDL_ReleaseGPUShader(m_device, vert);
    SDL_ReleaseGPUShader(m_device, frag);
    return false;
  }

  // pos(3) + uv(2) = 5 floats per vertex, must match Vertex in mesh.cpp
  SDL_GPUVertexBufferDescription vertDesc = {};
  vertDesc.slot = 0;
  vertDesc.pitch = sizeof(float) * 5;
  vertDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

  SDL_GPUVertexAttribute attribs[2] = {};
  attribs[0].location = 0;
  attribs[0].buffer_slot = 0;
  attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  attribs[0].offset = 0;
  attribs[1].location = 1;
  attribs[1].buffer_slot = 0;
  attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  attribs[1].offset = sizeof(float) * 3;

  SDL_GPUDepthStencilState depthState = {};
  depthState.enable_depth_test = true;
  depthState.enable_depth_write = true;
  depthState.compare_op = SDL_GPU_COMPAREOP_LESS;

  SDL_GPURasterizerState rasterState = {};
  rasterState.fill_mode = SDL_GPU_FILLMODE_FILL;
  rasterState.cull_mode =
      SDL_GPU_CULLMODE_NONE; // ground plane needs both faces
  // SDL_GPU Vulkan backend uses negative viewport height, which flips winding.
  // CCW world-space triangles appear CW in screen space → treat CW as front.
  rasterState.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

  SDL_GPUColorTargetDescription colorDesc = {};
  colorDesc.format = SDL_GetGPUSwapchainTextureFormat(m_device, window);

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.vertex_shader = vert;
  pipelineInfo.fragment_shader = frag;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertDesc;
  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_attributes = attribs;
  pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
  pipelineInfo.depth_stencil_state = depthState;
  pipelineInfo.rasterizer_state = rasterState;
  pipelineInfo.target_info.color_target_descriptions = &colorDesc;
  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.depth_stencil_format =
      SDL_GPU_TEXTUREFORMAT_D16_UNORM;
  pipelineInfo.target_info.has_depth_stencil_target = true;

  m_pipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipelineInfo);
  SDL_ReleaseGPUShader(m_device, vert);
  SDL_ReleaseGPUShader(m_device, frag);

  if (!m_pipeline) {
    std::cerr << "SDL_CreateGPUGraphicsPipeline failed: " << SDL_GetError()
              << '\n';
    return false;
  }

  int w, h;
  SDL_GetWindowSizeInPixels(window, &w, &h);

  SDL_GPUTextureCreateInfo depthInfo = {};
  depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
  depthInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
  depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
  depthInfo.width = static_cast<Uint32>(w);
  depthInfo.height = static_cast<Uint32>(h);
  depthInfo.layer_count_or_depth = 1;
  depthInfo.num_levels = 1;

  m_depthTexture = SDL_CreateGPUTexture(m_device, &depthInfo);
  if (!m_depthTexture) {
    std::cerr << "Depth texture creation failed: " << SDL_GetError() << '\n';
    return false;
  }

  return true;
}

void Renderer::render(SDL_GPUCommandBuffer* cmdBuf,
                      SDL_GPUTexture* swapchainTexture,
                      const std::vector<RenderObject>& objects) {
  SDL_GPUColorTargetInfo colorTarget = {};
  colorTarget.texture = swapchainTexture;
  colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
  colorTarget.store_op = SDL_GPU_STOREOP_STORE;
  colorTarget.clear_color = {0.53f, 0.81f, 0.98f, 1.0f}; // sky blue

  SDL_GPUDepthStencilTargetInfo depthTarget = {};
  depthTarget.texture = m_depthTexture;
  depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
  depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
  depthTarget.clear_depth = 1.0f;

  SDL_GPURenderPass* pass =
      SDL_BeginGPURenderPass(cmdBuf, &colorTarget, 1, &depthTarget);

  SDL_BindGPUGraphicsPipeline(pass, m_pipeline);

  for (const auto& obj : objects) {
    struct FragData {
      glm::vec4 color;
      float fogStrength;
      float fogStart;
      float fogEnd;
      float sideStart;
      float sideEnd;
      float _pad0 = 0.0f;
      float _pad1 = 0.0f;
      float _pad2 = 0.0f;
    };

    struct VertData {
      glm::mat4 modelView;
      glm::mat4 proj;
    };
    VertData vert = {obj.modelView, obj.proj};
    SDL_PushGPUVertexUniformData(cmdBuf, 0, &vert, sizeof(VertData));
    FragData frag = {obj.color,      m_fog.strength,  m_fog.depthStart,
                     m_fog.depthEnd, m_fog.sideStart, m_fog.sideEnd};
    SDL_PushGPUFragmentUniformData(cmdBuf, 0, &frag, sizeof(FragData));
    obj.mesh->draw(pass);
  }

  SDL_EndGPURenderPass(pass);
}
