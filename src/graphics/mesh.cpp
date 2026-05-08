#include "mesh.hpp"

#include <iostream>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct Vertex {
  float px, py, pz;
  float nx, ny, nz;
  float u, v;
};

// Two-step transfer buffer upload pattern.
//
// The GPU vertex buffer (SDL_GPUBuffer) lives in VRAM on discrete GPUs.
// VRAM is fast for the GPU to read but the CPU cannot write to it directly.
// To get data there we need a staging step:
//
//   1. Create a transfer buffer — this lives in CPU-visible memory (RAM or
//      a host-coherent heap) that the CPU can map and write into freely.
//   2. SDL_Map/Unmap: write vertex data into the transfer buffer from the CPU.
//   3. Record a copy pass: tell the GPU to DMA-copy from transfer buffer → VRAM.
//   4. Submit and release the transfer buffer — it's no longer needed after the copy.
//
// This is the standard Vulkan staging buffer pattern; SDL_GPU just names it clearly.
static SDL_GPUBuffer* uploadVertexBuffer(SDL_GPUDevice* device,
                                         const std::vector<Vertex>& vertices) {
  Uint32 size = static_cast<Uint32>(vertices.size() * sizeof(Vertex));

  // Final destination: GPU-side vertex buffer in VRAM.
  SDL_GPUBufferCreateInfo bufInfo = {};
  bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  bufInfo.size = size;
  SDL_GPUBuffer* buf = SDL_CreateGPUBuffer(device, &bufInfo);
  if (!buf) {
    return nullptr;
  }

  // Staging buffer: CPU-writable memory used once for the upload, then discarded.
  SDL_GPUTransferBufferCreateInfo tbInfo = {};
  tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  tbInfo.size = size;
  SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);
  if (!tb) {
    SDL_ReleaseGPUBuffer(device, buf);
    return nullptr;
  }

  // Map gives a raw CPU pointer into the staging buffer's memory.
  // SDL_memcpy then copies our vertex data into it.
  void* mapped = SDL_MapGPUTransferBuffer(device, tb, false);
  SDL_memcpy(mapped, vertices.data(), size);
  SDL_UnmapGPUTransferBuffer(device, tb);

  // A copy pass is like a render pass but for data movement: it records GPU
  // DMA commands (transfer buffer → vertex buffer) into a command buffer,
  // then submits them to the GPU queue. The CPU blocks until done because
  // we need the buffer ready before the first render frame.
  SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
  SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

  SDL_GPUTransferBufferLocation src = {};
  src.transfer_buffer = tb;

  SDL_GPUBufferRegion dst = {};
  dst.buffer = buf;
  dst.size = size;

  SDL_UploadToGPUBuffer(copy, &src, &dst, false);
  SDL_EndGPUCopyPass(copy);
  SDL_SubmitGPUCommandBuffer(cmd);
  SDL_ReleaseGPUTransferBuffer(device, tb);  // staging memory no longer needed

  return buf;
}

Mesh::Mesh(SDL_GPUDevice* device)
  : m_device(device) {}

Mesh::~Mesh() {
  if (m_vertexBuffer) {
    SDL_ReleaseGPUBuffer(m_device, m_vertexBuffer);
  }
}

std::unique_ptr<Mesh> Mesh::load(SDL_GPUDevice* device, const std::string& path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
    std::cerr << "Failed to load mesh: " << path << '\n';
    if (!err.empty()) {
      std::cerr << err << '\n';
    }
    return nullptr;
  }
  if (!warn.empty()) {
    std::cerr << "Mesh warning: " << warn << '\n';
  }

  std::vector<Vertex> vertices;
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex v = {};

      auto vi = static_cast<size_t>(index.vertex_index);
      v.px = attrib.vertices[3 * vi + 0];
      v.py = attrib.vertices[3 * vi + 1];
      v.pz = attrib.vertices[3 * vi + 2];

      if (index.normal_index >= 0) {
        auto ni = static_cast<size_t>(index.normal_index);
        v.nx = attrib.normals[3 * ni + 0];
        v.ny = attrib.normals[3 * ni + 1];
        v.nz = attrib.normals[3 * ni + 2];
      }

      if (index.texcoord_index >= 0) {
        auto ti = static_cast<size_t>(index.texcoord_index);
        v.u = attrib.texcoords[2 * ti + 0];
        v.v = attrib.texcoords[2 * ti + 1];
      }

      vertices.push_back(v);
    }
  }

  auto mesh = std::unique_ptr<Mesh>(new Mesh(device));
  mesh->m_vertexCount = static_cast<Uint32>(vertices.size());
  mesh->m_vertexBuffer = uploadVertexBuffer(device, vertices);

  if (!mesh->m_vertexBuffer) {
    std::cerr << "Vertex buffer upload failed\n";
    return nullptr;
  }

  return mesh;
}

bool Mesh::isValid() const {
  return m_vertexBuffer != nullptr;
}

void Mesh::draw(SDL_GPURenderPass* pass) const {
  SDL_GPUBufferBinding binding = {};
  binding.buffer = m_vertexBuffer;
  SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
  SDL_DrawGPUPrimitives(pass, m_vertexCount, 1, 0, 0);
}
