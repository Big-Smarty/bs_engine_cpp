#include "engine/renderer/renderer.hpp"
#include "vkfw/vkfw.hpp"
#include <engine/engine.hpp>
#include <memory>

namespace bs::engine {
Engine::Engine()
    : m_renderer(std::make_unique<renderer::Renderer>()),
      m_context(m_renderer->context()) {
  main_loop();
}
Engine::~Engine() {}

void Engine::main_loop() {
  while (!m_context->window().shouldClose()) {
    vkfw::pollEvents();
    m_renderer->render();
  }
}
} // namespace bs::engine
