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
                   SDL_GPU_SHADERSTAGE_FRAGMENT, 0);

  if (!vert || !frag) {
    SDL_ReleaseGPUShader(m_device, vert);
    SDL_ReleaseGPUShader(m_device, frag);
    return false;
  }

  // A graphics pipeline is a pre-compiled, immutable GPU state object.
  // Unlike OpenGL which has mutable global state (glEnable, glBlendFunc, etc.),
  // Vulkan/SDL_GPU bakes shaders + vertex layout + depth + rasterizer + output
  // format all into one object at creation time. The GPU driver can then
  // fully compile and optimize the shader for that fixed configuration.
  // Switching pipelines mid-frame is expensive — a real engine batches draw
  // calls by pipeline to minimize those switches.

  // Vertex buffer layout: tells the GPU how to step through the buffer.
  // pitch = stride in bytes between consecutive vertices (8 floats = 32 bytes).
  // VERTEX input_rate = advance one vertex per vertex (vs. INSTANCE = per
  // instance). This must exactly match the Vertex struct layout in mesh.cpp.
  SDL_GPUVertexBufferDescription vertexBufferDesc = {};
  vertexBufferDesc.slot = 0;
  vertexBufferDesc.pitch = sizeof(float) * 8;
  vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

  // Vertex attributes: map buffer byte offsets to shader input locations.
  // location = the number in the shader's `layout(location = N) in ...`.
  // offset = byte offset from the start of each vertex to this field.
  SDL_GPUVertexAttribute attributes[3] = {};
  attributes[0].location = 0; // inPosition in the vertex shader
  attributes[0].buffer_slot = 0;
  attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  attributes[0].offset = 0;
  attributes[1].location = 1; // inNormal
  attributes[1].buffer_slot = 0;
  attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  attributes[1].offset = sizeof(float) * 3;
  attributes[2].location = 2; // inTexCoord
  attributes[2].buffer_slot = 0;
  attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  attributes[2].offset = sizeof(float) * 6;

  // Depth test: each fragment's depth is compared against what's already in the
  // depth buffer. LESS means a fragment only wins (and overwrites) if it is
  // closer to the camera than whatever was drawn there before.
  // enable_depth_write must also be true, otherwise winning fragments don't
  // update the buffer and later draws get wrong results.
  SDL_GPUDepthStencilState depthState = {};
  depthState.enable_depth_test = true;
  depthState.enable_depth_write = true;
  depthState.compare_op = SDL_GPU_COMPAREOP_LESS;

  // Backface culling: triangles whose vertices wind counter-clockwise (CCW)
  // from the camera's perspective are considered front-facing. Triangles that
  // appear clockwise (i.e., you're looking at their back) are discarded before
  // the fragment shader runs — free performance gain for closed meshes.
  SDL_GPURasterizerState rasterState = {};
  rasterState.fill_mode = SDL_GPU_FILLMODE_FILL;
  rasterState.cull_mode = SDL_GPU_CULLMODE_BACK;
  rasterState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

  // The pipeline must know the exact pixel format of the render target it will
  // write to. We query it from the swapchain rather than hard-coding it,
  // because the format can differ between platforms and display configurations.
  SDL_GPUColorTargetDescription colorDesc = {};
  colorDesc.format = SDL_GetGPUSwapchainTextureFormat(m_device, window);

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.vertex_shader = vert;
  pipelineInfo.fragment_shader = frag;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions =
      &vertexBufferDesc;
  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_attributes = attributes;
  pipelineInfo.vertex_input_state.num_vertex_attributes = 3;
  pipelineInfo.depth_stencil_state = depthState;
  pipelineInfo.rasterizer_state = rasterState;
  pipelineInfo.target_info.color_target_descriptions = &colorDesc;
  pipelineInfo.target_info.num_color_targets = 1;
  // has_depth_stencil_target + format must match what render() passes as
  // depthTarget, or the pipeline is incompatible with the render pass and the
  // draw is undefined.
  pipelineInfo.target_info.depth_stencil_format =
      SDL_GPU_TEXTUREFORMAT_D16_UNORM;
  pipelineInfo.target_info.has_depth_stencil_target = true;

  // After SDL_CreateGPUGraphicsPipeline the shader objects are no longer
  // needed; their SPIR-V bytecode has been compiled into the pipeline.
  m_pipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipelineInfo);

  SDL_ReleaseGPUShader(m_device, vert);
  SDL_ReleaseGPUShader(m_device, frag);

  if (!m_pipeline) {
    std::cerr << "SDL_CreateGPUGraphicsPipeline failed: " << SDL_GetError()
              << '\n';
    return false;
  }

  // Depth buffer: a texture the same size as the window, one float per pixel.
  // D16_UNORM = 16-bit normalized depth values in [0, 1].
  // The GPU writes depth here during rendering and reads it back for the LESS
  // test. We use pixel size (not logical size) to match the swapchain
  // resolution exactly.
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
                      SDL_GPUTexture* swapchainTexture, const Mesh& mesh,
                      const glm::mat4& mvp) {
  // CLEAR load_op = discard whatever was in the texture last frame and fill
  // with the specified color. STORE store_op = keep the result so the swapchain
  // can display it. The depth buffer uses DONT_CARE for store because we never
  // need to read it after the frame — next frame starts with a fresh CLEAR
  // anyway.
  SDL_GPUColorTargetInfo colorTarget = {};
  colorTarget.texture = swapchainTexture;
  colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
  colorTarget.store_op = SDL_GPU_STOREOP_STORE;
  colorTarget.clear_color = {0.05f, 0.05f, 0.1f, 1.0f};

  // clear_depth = 1.0f fills the buffer with the maximum depth value (far
  // plane). Every fragment starts "infinitely far away" so the very first drawn
  // pixel always wins the depth test, and subsequent draws only win if they're
  // closer.
  SDL_GPUDepthStencilTargetInfo depthTarget = {};
  depthTarget.texture = m_depthTexture;
  depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
  depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
  depthTarget.clear_depth = 1.0f;

  // A render pass groups all draw calls that share the same render targets.
  // GPU drivers use this boundary to optimize memory bandwidth (tile-based GPUs
  // like mobile Adreno/Mali especially benefit from knowing load/store intent).
  SDL_GPURenderPass* pass =
      SDL_BeginGPURenderPass(cmdBuf, &colorTarget, 1, &depthTarget);

  SDL_BindGPUGraphicsPipeline(pass, m_pipeline);
  // Push constants: small uniform data uploaded inline into the command buffer.
  // Slot 0 = uniform binding 0 in the vertex shader (set=1, binding=0 in GLSL).
  // Cheaper than a descriptor update for per-frame data like the MVP matrix.
  SDL_PushGPUVertexUniformData(cmdBuf, 0, &mvp, sizeof(glm::mat4));
  mesh.draw(pass);

  SDL_EndGPURenderPass(pass);
}
