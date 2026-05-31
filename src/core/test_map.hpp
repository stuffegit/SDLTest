#pragma once

#include "mesh.hpp"
#include "renderer.hpp"
#include "settings.hpp"
#include "texture.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class TestMap {
  std::unique_ptr<Mesh> m_playerMesh;
  std::unique_ptr<Mesh> m_planeMesh;
  std::vector<std::unique_ptr<Mesh>> m_buildingMeshes;
  std::unique_ptr<Texture> m_buildingTexture;
  glm::vec3 m_playerPos = {0.0f, 0.0f, 0.0f};
  float m_cameraOffY = 5.0f;
  float m_cameraOffZ = 5.0f;
  float m_playerSpeed = 5.0f;

public:
  bool init(SDL_GPUDevice* device, const std::string& basePath);
  void configure(const Settings& settings);
  void update(double dt);
  void buildFrame(std::vector<RenderObject>& objects, glm::mat4& outView,
                  glm::mat4& outProj, float aspect);
};
