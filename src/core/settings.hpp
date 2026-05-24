#pragma once

#include "renderer.hpp"
#include <string>

struct Settings {
  FogParams fog = {1.0f, 9.0f, 28.0f, 5.0f, 10.0f};
  float cameraOffsetY = 5.0f;
  float cameraOffsetZ = 5.0f;
  float playerSpeed = 5.0f;
};

Settings loadSettings(const std::string& path);
void saveSettings(const Settings& s, const std::string& path);
