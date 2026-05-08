#pragma once

#include <cstdio>

// TODO: Keyboard debug info
// TODO: Mouse debug info
// TODO: Gamestates debug info

/** @brief Live frame timing values, updated once per frame by App. */
struct DebugInfo {
  float fps = 0.0f;
  float frameTimeMs = 0.0f;
};

/**
 * @brief Prints FPS and frame time to stdout, overwriting the same line.
 *
 * Placeholder until a proper in-game debug UI is added (e.g., Dear ImGui).
 */
class DebugOverlay {
public:
  void update(const DebugInfo& info) {
    printf("\r%-40s\r", "");
    printf("FPS: %3.0f Frame: %2.0f ms\r", info.fps, info.frameTimeMs);
    fflush(stdout);
  }
};
