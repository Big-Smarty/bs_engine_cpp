#pragma once

#include <engine/types/mesh.hpp>
#include <memory>

namespace bs::engine::types {
class Model {
public:
  Model();
  Model(const Mesh &mesh);
  ~Model();

  Model(const Model &other) : m_mesh(other.m_mesh) {}
  Model(Model &&other) : m_mesh(std::move(other.m_mesh)) {}
  Model &operator=(const Model &other) {
    m_mesh = other.m_mesh;
    return *this;
  }
  Model &operator=(Model &&other) {
    m_mesh = std::move(other.m_mesh);
    return *this;
  }

private:
  std::shared_ptr<Mesh> m_mesh;
};
} // namespace bs::engine::types
