#pragma once

#include "settings.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Scene {
public:
  virtual ~Scene() = default;
  virtual bool init(SDL_GPUDevice* device, const std::string& basePath) = 0;
  virtual void configure(const Settings& /*settings*/) {
  }
  virtual void update(double dt) = 0;
  virtual void buildFrame(std::vector<RenderObject>& objects,
                          glm::mat4& outView, glm::mat4& outProj,
                          float aspect) = 0;
};
