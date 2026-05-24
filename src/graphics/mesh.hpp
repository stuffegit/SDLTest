#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>

/**
 * @brief GPU-resident geometry loaded from an OBJ file.
 *
 * Vertices are stored flat (non-indexed) on the GPU: each triangle gets three
 * separate vertex entries even when positions are shared in the source file.
 * This is simpler to implement correctly; index welding can be added later for
 * bandwidth savings on large meshes.
 */
class Mesh {
  SDL_GPUDevice* m_device = nullptr;
  SDL_GPUBuffer* m_vertexBuffer = nullptr;
  Uint32 m_vertexCount = 0;

  /** @brief Private — use Mesh::load() to construct. */
  explicit Mesh(SDL_GPUDevice* device);

public:
  ~Mesh();

  /**
   * @brief Loads an OBJ file and uploads geometry to the GPU.
   *
   * @param device GPU device that will own the vertex buffer.
   * @param path   Absolute path to the .obj file.
   * @return nullptr on file-not-found or GPU upload failure.
   */
  [[nodiscard]] static std::unique_ptr<Mesh> load(SDL_GPUDevice* device,
                                                  const std::string& path);

  /** @brief Creates a flat XZ quad (Y=0) of given half-extents. */
  [[nodiscard]] static std::unique_ptr<Mesh>
  createPlane(SDL_GPUDevice* device, float halfW, float halfD);

  /** @brief Returns false if the vertex buffer failed to upload. */
  [[nodiscard]] bool isValid() const;

  /**
   * @brief Binds the vertex buffer and issues the draw call.
   *
   * Must be called inside an active render pass with a compatible pipeline
   * bound.
   */
  void draw(SDL_GPURenderPass* pass) const;
};
