#pragma once

#include <glm/glm.hpp>

namespace bs::engine::types {
struct CameraUBO {
  glm::mat4 model{};
  glm::mat4 view{};
  glm::mat4 proj{};
};
} // namespace bs::engine::types
