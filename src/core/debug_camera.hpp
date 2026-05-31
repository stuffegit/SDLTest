#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

class DebugCamera {
  glm::vec3 m_pos = {};
  float m_yaw = 0.f;
  float m_pitch = 0.f;
  float m_speed = 10.f;
  float m_sensitivity = 0.15f;
  bool m_active = false;

public:
  bool isActive() const {
    return m_active;
  }
  void toggle(SDL_Window* window, const glm::mat4& currentView);
  void handleEvent(const SDL_Event& e);
  void update(double dt);
  glm::mat4 viewMatrix() const;
};
