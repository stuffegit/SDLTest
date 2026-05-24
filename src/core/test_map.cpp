#include "test_map.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

bool TestMap::init(SDL_GPUDevice* device, const std::string& basePath) {
  m_cubeMesh = Mesh::load(device, basePath + "assets/meshes/cube.obj");
  if (!m_cubeMesh) {
    return false;
  }

  m_planeMesh = Mesh::createPlane(device, 20.0f, 20.0f);
  return m_planeMesh != nullptr;
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
  // No manual Y-flip: SDL_GPU's Vulkan backend uses negative viewport height
  // already
  outProj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

  auto mv = [&](const glm::mat4& model) { return outView * model; };

  static constexpr glm::vec4 kGround = {0.35f, 0.60f, 0.20f, 1.0f};
  static constexpr glm::vec4 kHouse = {0.55f, 0.38f, 0.18f, 1.0f};
  static constexpr glm::vec4 kPlayer = {0.00f, 0.00f, 0.00f, 1.0f};

  static const glm::vec3 kHousePositions[] = {
      {-3.0f, 0.0f, -4.0f},
      {3.0f, 0.0f, -4.0f},
      {-3.0f, 0.0f, -8.0f},
      {3.0f, 0.0f, -8.0f},
  };

  // Plane center shifted so near edge (z+4) stays in front of camera (at z+5)
  glm::mat4 groundModel = glm::translate(
      glm::mat4(1.0f), glm::vec3(m_playerPos.x, 0.0f, m_playerPos.z - 16.0f));
  objects.push_back({m_planeMesh.get(), mv(groundModel), outProj, kGround});

  // Houses: 1×1×1 cubes, center lifted to y=0.5 so they sit on the ground
  for (const auto& pos : kHousePositions) {
    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, 0.5f, 0.0f));
    objects.push_back({m_cubeMesh.get(), mv(model), outProj, kHouse});
  }

  glm::mat4 playerModel = glm::translate(
      glm::mat4(1.0f), m_playerPos + glm::vec3(0.0f, 0.25f, 0.0f));
  playerModel = glm::scale(playerModel, glm::vec3(0.5f));
  objects.push_back({m_cubeMesh.get(), mv(playerModel), outProj, kPlayer});
}
