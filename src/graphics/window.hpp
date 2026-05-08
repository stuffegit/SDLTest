#pragma once

#include <SDL3/SDL.h>
#include <string>

/**
 * @brief Thin RAII wrapper around an SDL_Window.
 *
 * Does not own a renderer — GPU presentation is handled by GPUContext.
 */
class Window {
  SDL_Window* m_window = nullptr;

public:
  /** @brief Creates and opens a window with the given title and pixel dimensions. */
  Window(const std::string& title, int width, int height);
  ~Window();

  /** @brief Returns false if SDL_CreateWindow failed. */
  [[nodiscard]] bool isValid() const;

  /**
   * @brief Raw SDL_Window pointer for subsystems that need it (GPUContext, etc.).
   *
   * Remains valid for the lifetime of this Window.
   */
  [[nodiscard]] SDL_Window* get() const { return m_window; }
};
