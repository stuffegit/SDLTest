#include "window.hpp"

Window::Window(const std::string& title, int width, int height) {
  m_window = SDL_CreateWindow(title.c_str(), width, height, 0);
}

Window::~Window() {
  if (m_window) {
    SDL_DestroyWindow(m_window);
  }
}

bool Window::isValid() const {
  return m_window != nullptr;
}
