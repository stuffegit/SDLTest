#include "mesh.hpp"

#include <iostream>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

static SDL_GPUBuffer* uploadVertexBuffer(SDL_GPUDevice* device,
                                         const std::vector<Vertex>& vertices) {
  auto size = static_cast<Uint32>(vertices.size() * sizeof(Vertex));

  SDL_GPUBufferCreateInfo bufInfo = {};
  bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  bufInfo.size = size;
  SDL_GPUBuffer* buf = SDL_CreateGPUBuffer(device, &bufInfo);
  if (!buf) {
    return nullptr;
  }

  SDL_GPUTransferBufferCreateInfo tbInfo = {};
  tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  tbInfo.size = size;
  SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);
  if (!tb) {
    SDL_ReleaseGPUBuffer(device, buf);
    return nullptr;
  }

  void* mapped = SDL_MapGPUTransferBuffer(device, tb, false);
  SDL_memcpy(mapped, vertices.data(), size);
  SDL_UnmapGPUTransferBuffer(device, tb);

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
  SDL_ReleaseGPUTransferBuffer(device, tb);

  return buf;
}

std::unique_ptr<Mesh> Mesh::createPlane(SDL_GPUDevice* device, float halfW,
                                        float halfD) {
  // CCW winding (normal +Y) matching the cube.obj convention
  std::vector<Vertex> verts = {
      {-halfW, 0, -halfD, 0, 0, 0, 1, 0}, {-halfW, 0, halfD, 0, 1, 0, 1, 0},
      {halfW, 0, halfD, 1, 1, 0, 1, 0},   {-halfW, 0, -halfD, 0, 0, 0, 1, 0},
      {halfW, 0, halfD, 1, 1, 0, 1, 0},   {halfW, 0, -halfD, 1, 0, 0, 1, 0},
  };
  auto mesh = std::unique_ptr<Mesh>(new Mesh(device));
  mesh->m_vertexCount = static_cast<Uint32>(verts.size());
  mesh->m_vertexBuffer = uploadVertexBuffer(device, verts);
  if (!mesh->m_vertexBuffer) {
    return nullptr;
  }
  return mesh;
}

Mesh::Mesh(SDL_GPUDevice* device) : m_device(device) {
}

Mesh::~Mesh() {
  if (m_vertexBuffer) {
    SDL_ReleaseGPUBuffer(m_device, m_vertexBuffer);
  }
}

std::unique_ptr<Mesh> Mesh::load(SDL_GPUDevice* device,
                                 const std::string& path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  std::string dir = path.substr(0, path.find_last_of("/\\") + 1);
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(),
                        dir.c_str())) {
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

      if (index.texcoord_index >= 0) {
        auto ti = static_cast<size_t>(index.texcoord_index);
        v.u = attrib.texcoords[2 * ti + 0];
        v.v = attrib.texcoords[2 * ti + 1];
      }

      if (index.normal_index >= 0) {
        auto ni = static_cast<size_t>(index.normal_index);
        v.nx = attrib.normals[3 * ni + 0];
        v.ny = attrib.normals[3 * ni + 1];
        v.nz = attrib.normals[3 * ni + 2];
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
