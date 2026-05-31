#include "settings.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Settings loadSettings(const std::string& path) {
  Settings s;
  std::ifstream f(path);
  if (!f) {
    return s;
  }
  try {
    json j = json::parse(f);
    s.fog.strength = j.value("fogStrength", s.fog.strength);
    s.fog.depthStart = j.value("fogDepthStart", s.fog.depthStart);
    s.fog.depthEnd = j.value("fogDepthEnd", s.fog.depthEnd);
    s.fog.sideStart = j.value("fogSideStart", s.fog.sideStart);
    s.fog.sideEnd = j.value("fogSideEnd", s.fog.sideEnd);
    s.cameraOffsetY = j.value("cameraOffsetY", s.cameraOffsetY);
    s.cameraOffsetZ = j.value("cameraOffsetZ", s.cameraOffsetZ);
    s.playerSpeed = j.value("playerSpeed", s.playerSpeed);
    s.light.dir.x = j.value("lightDirX", s.light.dir.x);
    s.light.dir.y = j.value("lightDirY", s.light.dir.y);
    s.light.dir.z = j.value("lightDirZ", s.light.dir.z);
    s.light.color.r = j.value("lightColorR", s.light.color.r);
    s.light.color.g = j.value("lightColorG", s.light.color.g);
    s.light.color.b = j.value("lightColorB", s.light.color.b);
    s.light.ambientColor.r = j.value("ambientR", s.light.ambientColor.r);
    s.light.ambientColor.g = j.value("ambientG", s.light.ambientColor.g);
    s.light.ambientColor.b = j.value("ambientB", s.light.ambientColor.b);
  } catch (...) {
  }
  return s;
}

void saveSettings(const Settings& s, const std::string& path) {
  json j;
  j["fogStrength"] = s.fog.strength;
  j["fogDepthStart"] = s.fog.depthStart;
  j["fogDepthEnd"] = s.fog.depthEnd;
  j["fogSideStart"] = s.fog.sideStart;
  j["fogSideEnd"] = s.fog.sideEnd;
  j["cameraOffsetY"] = s.cameraOffsetY;
  j["cameraOffsetZ"] = s.cameraOffsetZ;
  j["playerSpeed"] = s.playerSpeed;
  j["lightDirX"] = s.light.dir.x;
  j["lightDirY"] = s.light.dir.y;
  j["lightDirZ"] = s.light.dir.z;
  j["lightColorR"] = s.light.color.r;
  j["lightColorG"] = s.light.color.g;
  j["lightColorB"] = s.light.color.b;
  j["ambientR"] = s.light.ambientColor.r;
  j["ambientG"] = s.light.ambientColor.g;
  j["ambientB"] = s.light.ambientColor.b;
  std::ofstream f(path);
  if (f) {
    f << j.dump(2);
  }
}
