#include <engine/renderer/renderer.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

namespace bs::engine::renderer {
Renderer::Renderer()
    : m_context(std::make_unique<context::Context>()),
      m_camera(std::make_unique<camera::Camera>()) {
  try {
    m_command_pool =
        m_context->device().createCommandPool(vk::CommandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = 0,
        });
    m_command_buffer =
        m_context->device()
            .allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                .commandPool = m_command_pool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            })
            .front();
    auto [m_camera_ubo, m_camera_ubo_allocation] =
        m_context->allocator().createBuffer(
            vk::BufferCreateInfo{
                .size = sizeof(m_camera->camera_data()),
                .usage = vk::BufferUsageFlagBits::eUniformBuffer,
            },
            vma::AllocationCreateInfo{});

    m_pipeline_layouts.push_back(m_context->device().createPipelineLayout(
        vk::PipelineLayoutCreateInfo{}));
    m_pipelines.reserve(1);
    m_shader_modules.push_back(
        m_context->load_shader("./shaders/triangle.vert.spv"));
    m_shader_modules.push_back(
        m_context->load_shader("./shaders/triangle.frag.spv"));
    std::array<vk::PipelineShaderStageCreateInfo, 2>
        pipeline_shader_stage_create_infos{
            vk::PipelineShaderStageCreateInfo{
                .stage = vk::ShaderStageFlagBits::eVertex,
                .module = m_shader_modules[0],
                .pName = "main"},
            vk::PipelineShaderStageCreateInfo{
                .stage = vk::ShaderStageFlagBits::eFragment,
                .module = m_shader_modules[1],
                .pName = "main"},
        };
    vk::PipelineVertexInputStateCreateInfo
        pipeline_vertex_input_state_create_info{};
    vk::PipelineInputAssemblyStateCreateInfo
        pipeline_input_assembly_state_create_info{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };
    vk::PipelineViewportStateCreateInfo pipeline_viewport_state_create_info{
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };
    vk::PipelineRasterizationStateCreateInfo
        pipeline_rasterization_state_create_info{
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = false,
            .depthBiasConstantFactor = 0.f,
            .depthBiasClamp = 0.f,
            .depthBiasSlopeFactor = 0.f,
            .lineWidth = 1.f,
        };
    vk::PipelineMultisampleStateCreateInfo
        pipeline_multisample_state_create_info{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
        };
    vk::StencilOpState stencil_op_state{
        vk::StencilOp::eKeep,
        vk::StencilOp::eKeep,
        vk::StencilOp::eKeep,
        vk::CompareOp::eAlways,
    };
    vk::PipelineDepthStencilStateCreateInfo
        pipeline_depth_stencil_state_create_info{
            .depthTestEnable = true,
            .depthWriteEnable = true,
            .depthCompareOp = vk::CompareOp::eLessOrEqual,
            .depthBoundsTestEnable = false,
            .stencilTestEnable = false,
            .front = stencil_op_state,
            .back = stencil_op_state,
        };
    vk::ColorComponentFlags color_component_flags{
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state{
        .blendEnable = false,
        .srcColorBlendFactor = vk::BlendFactor::eZero,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eZero,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = color_component_flags,
    };
    vk::PipelineColorBlendStateCreateInfo
        pipeline_color_blend_state_create_info{
            .logicOpEnable = false,
            .logicOp = vk::LogicOp::eNoOp,
            .attachmentCount = 1,
            .pAttachments = &pipeline_color_blend_attachment_state,
            .blendConstants =
                {
                    {1.f, 1.f, 1.f, 1.f},
                },
        };
    std::array<vk::DynamicState, 2> dynamic_states{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
    };
    vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info{
        .stageCount = pipeline_shader_stage_create_infos.size(),
        .pStages = pipeline_shader_stage_create_infos.data(),
        .pVertexInputState = &pipeline_vertex_input_state_create_info,
        .pInputAssemblyState = &pipeline_input_assembly_state_create_info,
        .pTessellationState = nullptr,
        .pViewportState = &pipeline_viewport_state_create_info,
        .pRasterizationState = &pipeline_rasterization_state_create_info,
        .pMultisampleState = &pipeline_multisample_state_create_info,
        .pDepthStencilState = &pipeline_depth_stencil_state_create_info,
        .pColorBlendState = &pipeline_color_blend_state_create_info,
        .pDynamicState = &pipeline_dynamic_state_create_info,
        .layout = m_pipeline_layouts[0],
    };
    vk::Format format = m_context->color_attachment_format();
    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
        .depthAttachmentFormat = vk::Format::eD16Unorm,
        .stencilAttachmentFormat = vk::Format::eD16Unorm,
    };
    auto chained_pipeline_create_info =
        vk::StructureChain<vk::GraphicsPipelineCreateInfo,
                           vk::PipelineRenderingCreateInfo>(
            graphics_pipeline_create_info, pipeline_rendering_create_info)
            .get();
    auto result = m_context->device().createGraphicsPipelines(
        nullptr, 1, &chained_pipeline_create_info, nullptr, m_pipelines.data());

    switch (result) {
    case vk::Result::eSuccess:
      break;
    default:
      throw std::runtime_error("Failed to create graphics pipeline");
    }

    m_render_semaphore = m_context->device().createSemaphore({});
    m_present_semaphore = m_context->device().createSemaphore({});
    m_fence = m_context->device().createFence(vk::FenceCreateInfo{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    });
  } catch (std::exception &err) {
    spdlog::error("System error encountered: {0}", err.what());
    exit(-1);
  } catch (vk::SystemError &err) {
    spdlog::error("Vulkan error encountered: {0}", err.what());
    exit(-1);
  } catch (...) {
    spdlog::error("Unknown error encountered");
    exit(-1);
  }
}
Renderer::~Renderer() {
  m_context->device().freeCommandBuffers(m_command_pool, m_command_buffer);
  m_context->device().destroyCommandPool(m_command_pool);
  m_context->allocator().destroyBuffer(m_camera_ubo, m_camera_ubo_allocation);
  m_context->allocator().freeMemory(m_camera_ubo_allocation);
}

void Renderer::render() {
  auto result =
      m_context->device().waitForFences(1, &m_fence, true, 1000000000);

  switch (result) {
  case vk::Result::eSuccess:
    break;
  default:
    std::runtime_error("Error while waiting for fences");
  }

  result = m_context->device().resetFences(1, &m_fence);
  switch (result) {
  case vk::Result::eSuccess:
    break;
  default:
    std::runtime_error("Error while waiting for fences");
  }

  result = m_context->device().acquireNextImageKHR(
      m_context->swapchain(), 1000000000, m_present_semaphore, nullptr,
      &m_swapchain_image_index);
  switch (result) {
  case vk::Result::eSuccess:
    break;
  default:
    std::runtime_error("Error while waiting for fences");
  }
  m_command_buffer.reset();
  m_command_buffer.begin(vk::CommandBufferBeginInfo{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
  });

  const vk::ImageMemoryBarrier2 image_memory_barrier_rendering{
      .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .image = m_context->swapchain_images()[m_swapchain_image_index],
      .subresourceRange =
          {
              .aspectMask = vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  const vk::DependencyInfo dependency_info_rendering{
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &image_memory_barrier_rendering,
  };
  m_command_buffer.pipelineBarrier2(dependency_info_rendering);

  const vk::RenderingAttachmentInfo color_attachment_info{
      .imageView = m_context->swapchain_image_views()[m_swapchain_image_index],
      .imageLayout = vk::ImageLayout::eAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue =
          vk::ClearValue{
              .color =
                  {
                      std::array<float, 4>{1.f, 1.f, 1.f, 1.f},
                  },
          },
  };
  const vk::RenderingAttachmentInfoKHR depth_attachment_info{
      .imageView = m_context->depth_image_view(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue =
          vk::ClearValue{
              .depthStencil = vk::ClearDepthStencilValue{1.f, 0},
          },
  };
  const vk::RenderingInfo rendering_info{
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info,
      .pStencilAttachment = &depth_attachment_info,
  };

  m_command_buffer.beginRendering(rendering_info);

  m_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                m_pipelines[0]);
  m_command_buffer.setViewport(
      0, vk::Viewport{
             0.f, 0.f, static_cast<float>(m_context->window().getWidth()),
             static_cast<float>(m_context->window().getHeight()), 0.f, 0.f});
  m_command_buffer.setScissor(
      0,
      vk::Rect2D{vk::Offset2D{0, 0},
                 vk::Extent2D{
                     static_cast<uint32_t>(m_context->window().getWidth()),
                     static_cast<uint32_t>(m_context->window().getHeight())}});

  m_command_buffer.draw(3, 1, 0, 0);

  m_command_buffer.endRendering();

  const vk::ImageMemoryBarrier2 image_memory_barrier_presenting{
      .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .newLayout = vk::ImageLayout::ePresentSrcKHR,
      .image = m_context->swapchain_images()[m_swapchain_image_index],
      .subresourceRange =
          {
              .aspectMask = vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  const vk::DependencyInfo dependency_info_presenting{
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &image_memory_barrier_presenting,
  };
  m_command_buffer.pipelineBarrier2(dependency_info_presenting);

  m_command_buffer.end();

  vk::PipelineStageFlags wait_stage =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  m_context->queues()[0].submit(
      vk::SubmitInfo{
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &m_present_semaphore,
          .pWaitDstStageMask = &wait_stage,
          .commandBufferCount = 1,
          .pCommandBuffers = &m_command_buffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = &m_render_semaphore,
      },
      m_fence);

  const vk::PresentInfoKHR present_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_render_semaphore,
      .swapchainCount = 1,
      .pSwapchains = &m_context->swapchain(),
      .pImageIndices = &m_swapchain_image_index,
  };
  result = m_context->queues()[0].presentKHR(present_info);
  switch (result) {
  case vk::Result::eSuccess:
    break;
  default:
    std::runtime_error("Error while waiting for fences");
  }
}
} // namespace bs::engine::renderer
