#pragma once

#include <tuple>
#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.hpp>

#include <engine/types/vertex.hpp>

namespace bs::engine::types {
class Mesh {
public:
  Mesh();
  ~Mesh();

  Mesh(const Mesh &other)
      : m_vertices(other.m_vertices), m_allocation(other.m_allocation),
        m_vertex_buffer(other.m_vertex_buffer) {}
  Mesh(Mesh &&other)
      : m_vertices(std::move(other.m_vertices)),
        m_allocation(std::move(other.m_allocation)),
        m_vertex_buffer(std::move(other.m_vertex_buffer)) {}
  Mesh &operator=(const Mesh &other) {
    m_vertices = other.m_vertices;
    m_allocation = other.m_allocation;
    m_vertex_buffer = other.m_vertex_buffer;
    return *this;
  }
  Mesh &operator=(Mesh &&other) {
    m_vertices = std::move(other.m_vertices);
    m_allocation = std::move(other.m_allocation);
    m_vertex_buffer = std::move(other.m_vertex_buffer);
    return *this;
  }

  std::vector<Vertex> &vertices() { return m_vertices; }
  vma::Allocation allocation() { return m_allocation; }
  vk::Buffer vertex_buffer() { return m_vertex_buffer; }

private:
  std::vector<Vertex> m_vertices;
  vma::Allocation m_allocation;
  vk::Buffer m_vertex_buffer;
};
} // namespace bs::engine::types
