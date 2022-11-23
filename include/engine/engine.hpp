#pragma once

#include <engine/context/context.hpp>
#include <engine/renderer/renderer.hpp>

namespace bs::engine {
class Engine {
public:
  Engine();
  ~Engine();

  Engine(const Engine &) = delete;
  Engine(Engine &&) = delete;
  Engine &operator=(const Engine &) = delete;
  Engine &operator=(Engine &&) = delete;

private:
  void main_loop();

  std::unique_ptr<renderer::Renderer> m_renderer;
  std::unique_ptr<context::Context> &m_context;
};
} // namespace bs::engine
