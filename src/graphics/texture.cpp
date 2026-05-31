#include "texture.hpp"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static SDL_GPUTexture* uploadTexture(SDL_GPUDevice* device, const Uint8* pixels,
                                     int w, int h) {
  SDL_GPUTextureCreateInfo texInfo = {};
  texInfo.type = SDL_GPU_TEXTURETYPE_2D;
  texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
  texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
  texInfo.width = static_cast<Uint32>(w);
  texInfo.height = static_cast<Uint32>(h);
  texInfo.layer_count_or_depth = 1;
  texInfo.num_levels = 1;

  SDL_GPUTexture* tex = SDL_CreateGPUTexture(device, &texInfo);
  if (!tex) {
    return nullptr;
  }

  Uint32 size = static_cast<Uint32>(w * h * 4);

  SDL_GPUTransferBufferCreateInfo tbInfo = {};
  tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  tbInfo.size = size;
  SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);
  if (!tb) {
    SDL_ReleaseGPUTexture(device, tex);
    return nullptr;
  }

  void* mapped = SDL_MapGPUTransferBuffer(device, tb, false);
  SDL_memcpy(mapped, pixels, size);
  SDL_UnmapGPUTransferBuffer(device, tb);

  SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
  SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

  SDL_GPUTextureTransferInfo src = {};
  src.transfer_buffer = tb;
  src.pixels_per_row = static_cast<Uint32>(w);
  src.rows_per_layer = static_cast<Uint32>(h);

  SDL_GPUTextureRegion dst = {};
  dst.texture = tex;
  dst.w = static_cast<Uint32>(w);
  dst.h = static_cast<Uint32>(h);
  dst.d = 1;

  SDL_UploadToGPUTexture(copy, &src, &dst, false);
  SDL_EndGPUCopyPass(copy);
  SDL_SubmitGPUCommandBuffer(cmd);
  SDL_ReleaseGPUTransferBuffer(device, tb);

  return tex;
}

Texture::Texture(SDL_GPUDevice* device) : m_device(device) {
}

Texture::~Texture() {
  if (m_texture) {
    SDL_ReleaseGPUTexture(m_device, m_texture);
  }
}

std::unique_ptr<Texture> Texture::load(SDL_GPUDevice* device,
                                       const std::string& path) {
  int w, h, channels;
  stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
  if (!pixels) {
    std::cerr << "Failed to load texture: " << path << '\n';
    return nullptr;
  }

  auto tex = std::unique_ptr<Texture>(new Texture(device));
  tex->m_texture = uploadTexture(device, pixels, w, h);
  stbi_image_free(pixels);

  if (!tex->m_texture) {
    std::cerr << "Texture upload failed: " << path << '\n';
    return nullptr;
  }
  return tex;
}

std::unique_ptr<Texture> Texture::createWhite(SDL_GPUDevice* device) {
  const Uint8 pixels[4] = {255, 255, 255, 255};
  auto tex = std::unique_ptr<Texture>(new Texture(device));
  tex->m_texture = uploadTexture(device, pixels, 1, 1);
  return tex;
}
