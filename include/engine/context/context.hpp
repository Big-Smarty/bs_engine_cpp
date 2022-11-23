#pragma once

#include <vector>
#include <vk_mem_alloc.hpp>
#include <vkfw/vkfw.hpp>
#include <vulkan/vulkan.hpp>

namespace bs::engine::context {
class Context {
public:
  Context();
  ~Context();

  Context(const Context &) = delete;
  Context(Context &&) = delete;
  Context &operator=(const Context &) = delete;
  Context &operator=(Context &&) = delete;

  vk::ShaderModule load_shader(std::string shader_path);

  vk::Instance &instance() { return m_instance; }
  vk::PhysicalDevice &physical_device() { return m_physical_device; }
  vk::Device &device() { return m_device; }
  vk::SurfaceKHR &surface() { return m_surface; }
  vk::SwapchainKHR &swapchain() { return m_swapchain; }
  std::vector<vk::Image> &swapchain_images() { return m_swapchain_images; }
  std::vector<vk::ImageView> &swapchain_image_views() {
    return m_swapchain_image_views;
  }
  std::vector<vk::Queue> &queues() { return m_queues; }
  vk::Format color_attachment_format() { return m_color_attachment_format; }
  std::vector<vk::Buffer> &buffers() { return m_buffers; }
  vma::Allocator &allocator() { return m_allocator; }

  vk::Image &depth_image() { return m_depth_image; }
  vk::ImageView &depth_image_view() { return m_depth_image_view; }

  vkfw::Window &window() { return m_window; }

private:
  vk::Instance m_instance;
  vk::PhysicalDevice m_physical_device;
  vk::Device m_device;
  vk::SurfaceKHR m_surface;
  vk::SwapchainKHR m_swapchain;
  std::vector<vk::Image> m_swapchain_images;
  std::vector<vk::ImageView> m_swapchain_image_views;
  std::vector<vk::Queue> m_queues;

  vk::Format m_color_attachment_format;

  std::vector<vk::Buffer> m_buffers;
  std::vector<vma::Allocation> m_buffer_allocations;

  vma::Allocator m_allocator;

  vk::Image m_depth_image;
  vk::ImageView m_depth_image_view;
  vma::Allocation m_depth_image_allocation;

  vkfw::Window m_window;
  GLFWvidmode m_video_mode;
};
} // namespace bs::engine::context
