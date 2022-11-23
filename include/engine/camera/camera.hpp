#pragma once

#include <engine/types/camera_ubo.hpp>

namespace bs::engine::camera {
class Camera {
public:
  Camera();
  ~Camera();

  Camera(const Camera &) = delete;
  Camera(Camera &&) = delete;
  Camera &operator=(const Camera &) = delete;
  Camera &operator=(Camera &&) = delete;

  types::CameraUBO &camera_data() { return m_camera_data; }

private:
  types::CameraUBO m_camera_data;
};
} // namespace bs::engine::camera
