#include "renderer.hpp"

#include "texture.hpp"
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
                                   Uint32 numUniformBuffers,
                                   Uint32 numSamplers = 0) {
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
  info.num_samplers = numSamplers;

  SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
  if (!shader) {
    std::cerr << "SDL_CreateGPUShader failed: " << SDL_GetError() << '\n';
  }
  return shader;
}

Renderer::Renderer(SDL_GPUDevice* device) : m_device(device) {
}

Renderer::~Renderer() {
  if (m_sampler) {
    SDL_ReleaseGPUSampler(m_device, m_sampler);
  }
  if (m_depthTexture) {
    SDL_ReleaseGPUTexture(m_device, m_depthTexture);
  }
  if (m_pipeline) {
    SDL_ReleaseGPUGraphicsPipeline(m_device, m_pipeline);
  }
}

bool Renderer::init(SDL_Window* window, const std::string& basePath) {
  SDL_GPUShader* vert =
      createShader(m_device, basePath + "assets/shaders/mesh.vert.spv",
                   SDL_GPU_SHADERSTAGE_VERTEX, 1);
  SDL_GPUShader* frag =
      createShader(m_device, basePath + "assets/shaders/mesh.frag.spv",
                   SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

  if (!vert || !frag) {
    SDL_ReleaseGPUShader(m_device, vert);
    SDL_ReleaseGPUShader(m_device, frag);
    return false;
  }

  SDL_GPUVertexBufferDescription vertDesc = {};
  vertDesc.slot = 0;
  vertDesc.pitch = static_cast<Uint32>(sizeof(Vertex));
  vertDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

  SDL_GPUVertexAttribute attribs[3] = {};
  attribs[0].location = 0;
  attribs[0].buffer_slot = 0;
  attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  attribs[0].offset = static_cast<Uint32>(offsetof(Vertex, px));
  attribs[1].location = 1;
  attribs[1].buffer_slot = 0;
  attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  attribs[1].offset = static_cast<Uint32>(offsetof(Vertex, u));
  attribs[2].location = 2;
  attribs[2].buffer_slot = 0;
  attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  attribs[2].offset = static_cast<Uint32>(offsetof(Vertex, nx));

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
  pipelineInfo.vertex_input_state.num_vertex_attributes = 3;
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

  SDL_GPUSamplerCreateInfo samplerInfo = {};
  samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
  samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
  samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
  samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  m_sampler = SDL_CreateGPUSampler(m_device, &samplerInfo);
  if (!m_sampler) {
    std::cerr << "Sampler creation failed: " << SDL_GetError() << '\n';
    return false;
  }

  m_whiteTexture = Texture::createWhite(m_device);
  if (!m_whiteTexture) {
    std::cerr << "White texture creation failed\n";
    return false;
  }

  return true;
}

void Renderer::render(SDL_GPUCommandBuffer* cmdBuf,
                      SDL_GPUTexture* swapchainTexture,
                      const std::vector<RenderObject>& objects,
                      const glm::mat4& viewMatrix) {
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

  glm::vec3 lightDirVS = glm::normalize(glm::mat3(viewMatrix) * m_light.dir);

  for (const auto& obj : objects) {
    struct VertData {
      glm::mat4 modelView;
      glm::mat4 proj;
      glm::mat4 normalMatrix;
    };
    VertData vert = {
        obj.modelView,
        obj.proj,
        glm::mat4(glm::transpose(glm::inverse(glm::mat3(obj.modelView)))),
    };
    SDL_PushGPUVertexUniformData(cmdBuf, 0, &vert, sizeof(VertData));

    struct FragData {
      glm::vec4 color;
      float fogStrength, fogStart, fogEnd, sideStart, sideEnd;
      float _pad0, _pad1, _pad2;
      glm::vec4 lightDir;
      glm::vec4 lightColor;
      glm::vec4 ambientColor;
      float shininess, specStrength;
      float _lpad0, _lpad1;
    };
    FragData frag = {
        obj.color,
        m_fog.strength,
        m_fog.depthStart,
        m_fog.depthEnd,
        m_fog.sideStart,
        m_fog.sideEnd,
        0.0f,
        0.0f,
        0.0f,
        glm::vec4(lightDirVS, 0.0f),
        glm::vec4(m_light.color, 0.0f),
        glm::vec4(m_light.ambientColor, 0.0f),
        obj.shininess,
        obj.specStrength,
        0.0f,
        0.0f,
    };
    SDL_PushGPUFragmentUniformData(cmdBuf, 0, &frag, sizeof(FragData));

    SDL_GPUTextureSamplerBinding texBinding = {};
    texBinding.texture =
        obj.texture ? obj.texture->get() : m_whiteTexture->get();
    texBinding.sampler = m_sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &texBinding, 1);

    obj.mesh->draw(pass);
  }

  SDL_EndGPURenderPass(pass);
}
