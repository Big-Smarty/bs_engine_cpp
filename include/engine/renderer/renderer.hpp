#pragma once

#include <engine/camera/camera.hpp>
#include <engine/context/context.hpp>
#include <engine/types/camera_ubo.hpp>
#include <memory>

namespace bs::engine::renderer {
class Renderer {
public:
  Renderer();
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer(Renderer &&) = delete;
  Renderer &operator=(const Renderer &) = delete;
  Renderer &operator=(Renderer &&) = delete;

  std::unique_ptr<camera::Camera> &camera() { return m_camera; }
  std::unique_ptr<context::Context> &context() { return m_context; }

  void render();

private:
  std::unique_ptr<context::Context> m_context;
  std::unique_ptr<camera::Camera> m_camera;
  vk::Buffer m_camera_ubo;
  vma::Allocation m_camera_ubo_allocation;

  vk::CommandPool m_command_pool;
  vk::CommandBuffer m_command_buffer;

  std::vector<vk::ShaderModule> m_shader_modules;

  std::vector<vk::DescriptorSetLayoutBinding> m_descriptor_set_layout_bindings;
  std::vector<vk::DescriptorSetLayout> m_descriptor_set_layout;
  std::vector<vk::DescriptorSet> m_descriptor_sets;
  std::vector<vk::PipelineLayout> m_pipeline_layouts;
  std::vector<vk::Pipeline> m_pipelines;

  vk::Semaphore m_render_semaphore;
  vk::Semaphore m_present_semaphore;
  vk::Fence m_fence;

  uint32_t m_swapchain_image_index;
};
} // namespace bs::engine::renderer
