#include "test_map.hpp"

#include "texture.hpp"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

static constexpr const char* kBuildingNames[] = {
    "building-a", "building-b", "building-c", "building-d",
    "building-e", "building-f", "building-g", "building-h",
};
static constexpr int kNumBuildings =
    static_cast<int>(sizeof(kBuildingNames) / sizeof(kBuildingNames[0]));

bool TestMap::init(SDL_GPUDevice* device, const std::string& basePath) {
  m_playerMesh = Mesh::load(device, basePath + "assets/meshes/cube.obj");
  if (!m_playerMesh) {
    return false;
  }

  m_planeMesh = Mesh::createPlane(device, 20.0f, 20.0f);
  if (!m_planeMesh) {
    return false;
  }

  const std::string buildingDir = basePath + "assets/meshes/buildings/";
  for (int i = 0; i < kNumBuildings; ++i) {
    auto mesh = Mesh::load(device, buildingDir + kBuildingNames[i] + ".obj");
    if (!mesh) {
      std::cerr << "Failed to load " << kBuildingNames[i] << '\n';
      return false;
    }
    m_buildingMeshes.push_back(std::move(mesh));
  }

  m_buildingTexture = Texture::load(
      device, basePath + "assets/meshes/buildings/Textures/colormap.png");
  if (!m_buildingTexture) {
    return false;
  }

  return true;
}

void TestMap::configure(const Settings& settings) {
  m_cameraOffY = settings.cameraOffsetY;
  m_cameraOffZ = settings.cameraOffsetZ;
  m_playerSpeed = settings.playerSpeed;
}

void TestMap::update(double dt) {
  const auto* keys = SDL_GetKeyboardState(nullptr);
  float speed = m_playerSpeed * static_cast<float>(dt);

  if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
    m_playerPos.z -= speed;
  }
  if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
    m_playerPos.z += speed;
  }
  if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
    m_playerPos.x -= speed;
  }
  if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
    m_playerPos.x += speed;
  }
}

void TestMap::buildFrame(std::vector<RenderObject>& objects, glm::mat4& outView,
                         glm::mat4& outProj, float aspect) {
  glm::vec3 cameraOffset = {0.0f, m_cameraOffY, m_cameraOffZ};
  outView = glm::lookAt(m_playerPos + cameraOffset, m_playerPos,
                        glm::vec3(0.0f, 1.0f, 0.0f));
  outProj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

  auto mv = [&](const glm::mat4& model) { return outView * model; };

  static constexpr glm::vec4 kGround = {0.35f, 0.60f, 0.20f, 1.0f};
  static constexpr glm::vec4 kBuilding = {0.72f, 0.65f, 0.54f, 1.0f};
  static constexpr glm::vec4 kPlayer = {0.10f, 0.10f, 0.12f, 1.0f};

  // Ground — matte
  glm::mat4 groundModel = glm::translate(
      glm::mat4(1.0f), glm::vec3(m_playerPos.x, 0.0f, m_playerPos.z - 16.0f));
  objects.push_back(
      {m_planeMesh.get(), mv(groundModel), outProj, kGround, 4.0f, 0.0f});

  // Buildings — two rows of four, origin at base so no y offset needed
  static constexpr glm::vec3 kBuildingPositions[kNumBuildings] = {
      {-9.0f, 0.0f, -5.0f}, {-3.0f, 0.0f, -5.0f},  {3.0f, 0.0f, -5.0f},
      {9.0f, 0.0f, -5.0f},  {-9.0f, 0.0f, -12.0f}, {-3.0f, 0.0f, -12.0f},
      {3.0f, 0.0f, -12.0f}, {9.0f, 0.0f, -12.0f},
  };
  for (int i = 0; i < kNumBuildings; ++i) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), kBuildingPositions[i]);
    objects.push_back({m_buildingMeshes[static_cast<size_t>(i)].get(),
                       mv(model), outProj, kBuilding, 64.0f, 0.6f,
                       m_buildingTexture.get()});
  }

  // Player — glossy cube
  glm::mat4 playerModel = glm::translate(
      glm::mat4(1.0f), m_playerPos + glm::vec3(0.0f, 0.25f, 0.0f));
  playerModel = glm::scale(playerModel, glm::vec3(0.5f));
  objects.push_back(
      {m_playerMesh.get(), mv(playerModel), outProj, kPlayer, 32.0f, 0.4f});
}
