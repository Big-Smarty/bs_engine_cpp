#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <exception>
#define VMA_IMPLEMENTATION
#include <engine/context/context.hpp>

#include "vulkan/vulkan_handles.hpp"
#include <vulkan/vulkan_enums.hpp>

#include <spdlog/spdlog.h>

#include <fmt/format.h>
#include <fstream>

namespace bs::engine::context {
Context::Context() {
  try {
    vkfw::init();
    vkfw::Monitor monitor = vkfw::getPrimaryMonitor();
    m_video_mode = *monitor.getVideoMode();
    vkfw::WindowHints window_hints;
    window_hints.clientAPI = vkfw::ClientAPI::eNone;
    window_hints.floating = true;
    m_window = vkfw::createWindow(m_video_mode.width, m_video_mode.height,
                                  "BS Engine", window_hints, monitor);

    auto instance_extensions = vkfw::getRequiredInstanceExtensions();
    std::vector<const char *> instance_layers;
    instance_layers.emplace_back("VK_LAYER_KHRONOS_validation");
    // instance_layers.emplace_back("VK_LAYER_LUNARG_api_dump");
    vk::ApplicationInfo app_info{.apiVersion = VK_API_VERSION_1_3};
    vk::InstanceCreateInfo instance_create_info{
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(instance_layers.size()),
        .ppEnabledLayerNames = instance_layers.data(),
        .enabledExtensionCount =
            static_cast<uint32_t>(instance_extensions.size()),
        .ppEnabledExtensionNames = instance_extensions.data(),
    };
    m_instance = vk::createInstance(instance_create_info);

    m_physical_device = m_instance.enumeratePhysicalDevices().front();

    float queue_priority = 0.f;
    vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = 0,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    std::vector<const char *> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");
    device_extensions.push_back("VK_KHR_dynamic_rendering");
    vk::PhysicalDeviceVulkan13Features vulkan_13_features{
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    vk::DeviceCreateInfo device_create_info{
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount =
            static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    };
    m_device = m_physical_device.createDevice(
        vk::StructureChain<vk::DeviceCreateInfo,
                           vk::PhysicalDeviceVulkan13Features>(
            device_create_info, vulkan_13_features)
            .get());
    m_queues.reserve(1);
    m_device.getQueue(0, 0, &m_queues[0]);

    m_allocator = vma::createAllocator(vma::AllocatorCreateInfo{
        .physicalDevice = m_physical_device,
        .device = m_device,
        .instance = m_instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    });

    m_surface = vkfw::createWindowSurface(m_instance, m_window);
    std::vector<vk::SurfaceFormatKHR> surface_formats =
        m_physical_device.getSurfaceFormatsKHR(m_surface);
    assert(!surface_formats.empty());
    m_color_attachment_format =
        (surface_formats[0].format == vk::Format::eUndefined)
            ? vk::Format::eB8G8R8A8Unorm
            : surface_formats[0].format;
    vk::SurfaceCapabilitiesKHR surface_capabilities =
        m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
    vk::SurfaceTransformFlagBitsKHR pre_transform =
        (surface_capabilities.supportedTransforms &
         vk::SurfaceTransformFlagBitsKHR::eIdentity)
            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
            : surface_capabilities.currentTransform;

    vk::CompositeAlphaFlagBitsKHR composite_alpha =
        (surface_capabilities.supportedCompositeAlpha &
         vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
            ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
        : (surface_capabilities.supportedCompositeAlpha &
           vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
            ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
        : (surface_capabilities.supportedCompositeAlpha &
           vk::CompositeAlphaFlagBitsKHR::eInherit)
            ? vk::CompositeAlphaFlagBitsKHR::eInherit
            : vk::CompositeAlphaFlagBitsKHR::eOpaque;

    m_swapchain = m_device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
        .surface = m_surface,
        .minImageCount = surface_capabilities.minImageCount,
        .imageFormat = m_color_attachment_format,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent =
            {
                static_cast<uint32_t>(m_video_mode.width),
                static_cast<uint32_t>(m_video_mode.height),
            },
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = pre_transform,
        .compositeAlpha = composite_alpha,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true,
        .oldSwapchain = nullptr,
    });

    m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);
    m_swapchain_image_views.reserve(m_swapchain_images.size());
    for (auto image : m_swapchain_images) {
      vk::ImageViewCreateInfo image_view_create_info{
          .image = image,
          .viewType = vk::ImageViewType::e2D,
          .format = m_color_attachment_format,
          .components = {},
          .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
      m_swapchain_image_views.push_back(
          m_device.createImageView(image_view_create_info));
    }

    const vk::Format depth_format = vk::Format::eD16Unorm;
    vk::FormatProperties format_properties =
        m_physical_device.getFormatProperties(depth_format);

    vk::ImageTiling tiling;
    if (format_properties.linearTilingFeatures &
        vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
      tiling = vk::ImageTiling::eLinear;
    } else if (format_properties.optimalTilingFeatures &
               vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
      tiling = vk::ImageTiling::eOptimal;
    } else {
      throw std::runtime_error(
          "DepthStencilAttachment is not supported for D16Unorm depth format.");
    }
    vk::ImageCreateInfo depth_image_create_info{
        .imageType = vk::ImageType::e2D,
        .format = depth_format,
        .extent =
            vk::Extent3D{.width = static_cast<uint32_t>(m_video_mode.width),
                         .height = static_cast<uint32_t>(m_video_mode.height),
                         .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
    };

    vma::AllocationCreateInfo depth_image_allocation_info{
        .flags = vma::AllocationCreateFlagBits::eDedicatedMemory,
        .usage = vma::MemoryUsage::eAuto,
        .priority = 1.f,
    };
    auto [m_depth_image, m_depth_image_allocation] = m_allocator.createImage(
        depth_image_create_info, depth_image_allocation_info);

    m_depth_image_view = m_device.createImageView(vk::ImageViewCreateInfo{
        .image = m_depth_image,
        .viewType = vk::ImageViewType::e2D,
        .format = depth_format,
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    });
  } catch (vk::SystemError &err) {
    spdlog::error("Caught vulkan system error: {0}", err.what());
    exit(-1);
  } catch (std::exception &err) {
    spdlog::error("Caught exception: {0}", err.what());
    exit(-1);
  } catch (...) {
    spdlog::error("Caught unknown error");
    exit(-1);
  }
}
Context::~Context() {
  m_allocator.destroyImage(m_depth_image, m_depth_image_allocation);
  m_allocator.freeMemory(m_depth_image_allocation);
  for (int i = 0; i < m_buffers.size(); i++) {
    m_allocator.destroyBuffer(m_buffers[i], m_buffer_allocations[i]);
    m_allocator.freeMemory(m_buffer_allocations[i]);
  }
  m_allocator.destroy();
  for (auto &i : m_swapchain_image_views) {
    m_device.destroyImageView(i);
  }
  for (auto &i : m_swapchain_images) {
    m_device.destroyImage(i);
  }
  m_device.destroySwapchainKHR(m_swapchain);
  m_instance.destroySurfaceKHR(m_surface);
  m_device.destroy();
  m_instance.destroy();
}

vk::ShaderModule Context::load_shader(std::string shader_path) {
  vk::ShaderModule out;
  try {

    std::ifstream shader_file(shader_path, std::ios::ate | std::ios::binary);
    if (!shader_file.is_open())
      throw std::runtime_error(
          fmt::format("Failed to open shader file: {0}", shader_path));

    auto shader_file_size = shader_file.tellg();
    shader_file.seekg(0);
    std::vector<uint32_t> buffer(shader_file_size / sizeof(uint32_t));
    shader_file.read(reinterpret_cast<char *>(buffer.data()), shader_file_size);
    shader_file.close();

    out = m_device.createShaderModule(vk::ShaderModuleCreateInfo{
        .codeSize = static_cast<size_t>(shader_file_size),
        .pCode = buffer.data(),
    });
  } catch (std::exception &err) {
    spdlog::error("Encountered a system error: {0}", err.what());
    exit(-1);
  } catch (vk::SystemError &err) {
    spdlog::error("Encountered a vulkan error: {0}", err.what());
    exit(-1);
  } catch (...) {
    spdlog::error("Encountered an unknown error");
    exit(-1);
  }
  return out;
}
} // namespace bs::engine::context
