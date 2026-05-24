#pragma once

#include "mesh.hpp"
#include "scene.hpp"
#include <glm/glm.hpp>
#include <memory>

class TestMap final : public Scene {
  std::unique_ptr<Mesh> m_cubeMesh;
  std::unique_ptr<Mesh> m_planeMesh;
  glm::vec3 m_playerPos = {0.0f, 0.0f, 0.0f};
  float m_cameraOffY = 5.0f;
  float m_cameraOffZ = 5.0f;
  float m_playerSpeed = 5.0f;

public:
  bool init(SDL_GPUDevice* device, const std::string& basePath) override;
  void configure(const Settings& settings) override;
  void update(double dt) override;
  void buildFrame(std::vector<RenderObject>& objects, glm::mat4& outView,
                  glm::mat4& outProj, float aspect) override;
};
