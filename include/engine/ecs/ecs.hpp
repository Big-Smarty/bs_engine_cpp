#pragma once

namespace bs::engine::ecs {
class ECS {
  ECS();
  ~ECS();

  ECS(const ECS &) = delete;
  ECS(ECS &&) = delete;
  ECS &operator=(const ECS &) = delete;
  ECS &operator=(ECS &&) = delete;
};
} // namespace bs::engine::ecs
