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
  std::ofstream f(path);
  if (f) {
    f << j.dump(2);
  }
}
