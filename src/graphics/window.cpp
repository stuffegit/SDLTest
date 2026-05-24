#include "window.hpp"
#include "SDL3/SDL_video.h"

Window::Window(const std::string& title, int width, int height) {
  m_window =
      SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_BORDERLESS);
}

Window::~Window() {
  if (m_window) {
    SDL_DestroyWindow(m_window);
  }
}

bool Window::isValid() const {
  return m_window != nullptr;
}
