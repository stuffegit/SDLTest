#include "debug_camera.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

static glm::vec3 calcFront(float yawDeg, float pitchDeg) {
  float yr = glm::radians(yawDeg);
  float pr = glm::radians(pitchDeg);
  return glm::normalize(glm::vec3(std::cos(yr) * std::cos(pr), std::sin(pr),
                                  std::sin(yr) * std::cos(pr)));
}

void DebugCamera::toggle(SDL_Window* window, const glm::mat4& currentView) {
  m_active = !m_active;
  SDL_SetWindowRelativeMouseMode(window, m_active);

  if (m_active) {
    glm::mat4 inv = glm::inverse(currentView);
    m_pos = glm::vec3(inv[3]);
    glm::vec3 fwd = -glm::vec3(inv[2]);
    m_pitch = glm::degrees(std::asin(glm::clamp(fwd.y, -1.f, 1.f)));
    m_yaw = glm::degrees(std::atan2(fwd.z, fwd.x));
  }
}

void DebugCamera::handleEvent(const SDL_Event& e) {
  if (!m_active) {
    return;
  }
  if (e.type == SDL_EVENT_MOUSE_MOTION) {
    m_yaw += e.motion.xrel * m_sensitivity;
    m_pitch -= e.motion.yrel * m_sensitivity;
    m_pitch = glm::clamp(m_pitch, -89.f, 89.f);
  }
}

void DebugCamera::update(double dt) {
  if (!m_active) {
    return;
  }
  float speed = m_speed * static_cast<float>(dt);
  glm::vec3 front = calcFront(m_yaw, m_pitch);
  glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.f, 1.f, 0.f)));

  const auto* keys = SDL_GetKeyboardState(nullptr);
  if (keys[SDL_SCANCODE_W]) {
    m_pos += front * speed;
  }
  if (keys[SDL_SCANCODE_S]) {
    m_pos -= front * speed;
  }
  if (keys[SDL_SCANCODE_A]) {
    m_pos -= right * speed;
  }
  if (keys[SDL_SCANCODE_D]) {
    m_pos += right * speed;
  }
  if (keys[SDL_SCANCODE_SPACE]) {
    m_pos.y += speed;
  }
  if (keys[SDL_SCANCODE_LCTRL]) {
    m_pos.y -= speed;
  }
}

glm::mat4 DebugCamera::viewMatrix() const {
  glm::vec3 front = calcFront(m_yaw, m_pitch);
  return glm::lookAt(m_pos, m_pos + front, glm::vec3(0.f, 1.f, 0.f));
}
