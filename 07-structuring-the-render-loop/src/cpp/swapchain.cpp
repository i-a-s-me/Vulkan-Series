// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <core.hpp>
#include <swapchain.hpp>

Result<Box<Swapchain>> Swapchain::create(const VulkanCore *core, VkExtent2D initial_extent)
{
  Box<Swapchain> swapchain(new Swapchain(core));

  const auto surface_formats = vkEnumCall<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR,
                                                              core->get_physical_device(), core->get_surface());
  const auto surface_present_modes = vkEnumCall<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR,
                                                                  core->get_physical_device(), core->get_surface());
  assert(surface_formats.size());
  assert(surface_present_modes.size());
  swapchain->m_format = surface_formats[0];
  swapchain->m_present_mode = (std::find(surface_present_modes.begin(), surface_present_modes.end(),
                                         VK_PRESENT_MODE_MAILBOX_KHR) != surface_present_modes.end())
                                  ? VK_PRESENT_MODE_MAILBOX_KHR
                                  : VK_PRESENT_MODE_FIFO_KHR;

  swapchain->m_pending_extent = initial_extent;

  PROPAGATE_RESULT_ERROR(swapchain->recreate());
  PROPAGATE_RESULT_ERROR(swapchain->create_frame_resources());

  return swapchain;
}

Swapchain::~Swapchain()
{
  if (!m_core || !m_core->get_device())
    return;

  m_depth_image.reset();

  for (uint32_t i = 0; i < NUM_CONCURRENT_FRAMES; i++)
  {
    vkDestroyFence(m_core->get_device(), m_frames[i].fen_in_flight, nullptr);
    vkDestroySemaphore(m_core->get_device(), m_frames[i].sm_image_available, nullptr);
  }

  for (size_t i = 0; i < m_render_done_semaphores.size(); i++)
    vkDestroySemaphore(m_core->get_device(), m_render_done_semaphores[i], nullptr);
  m_render_done_semaphores.clear();

  vkDestroyCommandPool(m_core->get_device(), m_frame_resource_command_pool, nullptr);

  for (const auto &view : m_image_views)
    vkDestroyImageView(m_core->get_device(), view, nullptr);
  vkDestroySwapchainKHR(m_core->get_device(), m_swapchain, nullptr);
}

void Swapchain::resize(uint32_t fb_width, uint32_t fb_height)
{
  m_pending_extent = {fb_width, fb_height};
  request_recreation();
}

void Swapchain::request_recreation()
{
  m_is_out_of_date = true;
}

Result<> Swapchain::update()
{
  const auto &current_frame = m_frames[m_current_frame_index];

  vkWaitForFences(m_core->get_device(), 1, &current_frame.fen_in_flight, VK_TRUE, UINT64_MAX);

  uint32_t image_index{};
  switch (vkAcquireNextImageKHR(m_core->get_device(), m_swapchain, UINT64_MAX, current_frame.sm_image_available,
                                VK_NULL_HANDLE, &image_index))
  {
  case VK_ERROR_OUT_OF_DATE_KHR:
    PROPAGATE_RESULT_ERROR(recreate());
    return Ok();

  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    break;

  default:
    return "Failed to acquire swapchain image";
  }

  VkCommandBufferBeginInfo cmd_begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkResetCommandBuffer(current_frame.cmd_buffer, 0);
  if (vkBeginCommandBuffer(current_frame.cmd_buffer, &cmd_begin_info) != VK_SUCCESS)
    return "Failed to start recording command buffer";

  if (!Image::transition_image_layout(current_frame.cmd_buffer, m_images[image_index], VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
    return "Failed to transition swapchain image";

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_image_views[image_index],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue =
          {
              .color =
                  {
                      0.1f,
                      0.1f,
                      0.1f,
                      1.0f,
                  },
          },
  };

  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_depth_image->get_view(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue =
          {
              .depthStencil =
                  {
                      1.0f,
                      0,
                  },
          },
  };

  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea =
          {
              {},
              m_extent,
          },
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info,
  };

  vkCmdBeginRendering(current_frame.cmd_buffer, &rendering_info);

  VkViewport viewport = {
      .width = (float) m_extent.width,
      .height = (float) m_extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(current_frame.cmd_buffer, 0, 1, &viewport);

  VkRect2D scissor{
      .extent = m_extent,
  };
  vkCmdSetScissor(current_frame.cmd_buffer, 0, 1, &scissor);

  // draw_frame(, m_image_views[image_index]);

  vkCmdEndRendering(current_frame.cmd_buffer);

  if (!Image::transition_image_layout(current_frame.cmd_buffer, m_images[image_index],
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    return "Failed to transition swapchain image";

  if (vkEndCommandBuffer(current_frame.cmd_buffer) != VK_SUCCESS)
    return "Failed to finish recording command buffer";

  VkPipelineStageFlags submit_pipeline_wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &current_frame.sm_image_available,
      .pWaitDstStageMask = &submit_pipeline_wait_stage,
      .commandBufferCount = 1,
      .pCommandBuffers = &current_frame.cmd_buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_render_done_semaphores[image_index],
  };

  vkResetFences(m_core->get_device(), 1, &current_frame.fen_in_flight);
  if (vkQueueSubmit(m_core->get_queue_graphics(), 1, &submit_info, current_frame.fen_in_flight) != VK_SUCCESS)
    return "Failed to submit frame command buffer";

  VkPresentInfoKHR present_info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_render_done_semaphores[image_index],
      .swapchainCount = 1,
      .pSwapchains = &m_swapchain,
      .pImageIndices = &image_index,
  };

  switch (vkQueuePresentKHR(m_core->get_queue_present(), &present_info))
  {
  case VK_ERROR_OUT_OF_DATE_KHR:
  case VK_SUBOPTIMAL_KHR:
    PROPAGATE_RESULT_ERROR(recreate());
    break;

  case VK_SUCCESS:
    break;

  default:
    return "Failed to present swapchain";
  }

  if (m_is_out_of_date)
    PROPAGATE_RESULT_ERROR(recreate());

  m_current_frame_index = (m_current_frame_index + 1) % NUM_CONCURRENT_FRAMES;

  return Ok();
}

Result<> Swapchain::recreate()
{
  m_core->wait_idle();

  VkSurfaceCapabilitiesKHR surface_caps{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_core->get_physical_device(), m_core->get_surface(), &surface_caps) !=
      VK_SUCCESS)
    return "Failed to query physical device surface caps";

  // A minimized window has a 0x0 surface, and a 0x0 swapchain is not allowed. So we
  // keep the current swapchain, and try again later (once the window is restored).
  if ((surface_caps.maxImageExtent.width == 0) || (surface_caps.maxImageExtent.height == 0))
  {
    m_is_out_of_date = true;
    return Ok();
  }

  m_image_count = std::min(surface_caps.minImageCount + 1,
                           (surface_caps.maxImageCount == 0) ? UINT32_MAX : surface_caps.maxImageCount);

  m_pre_transform = surface_caps.currentTransform;

  m_pending_extent.width =
      std::clamp(m_pending_extent.width, surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width);
  m_pending_extent.height =
      std::clamp(m_pending_extent.height, surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);
  m_extent = m_pending_extent;

  m_depth_image.reset();
  for (const auto &view : m_image_views)
    vkDestroyImageView(m_core->get_device(), view, nullptr);
  if (m_swapchain)
    vkDestroySwapchainKHR(m_core->get_device(), m_swapchain, nullptr);
  m_image_views.clear();
  m_swapchain = VK_NULL_HANDLE;

  VkSwapchainCreateInfoKHR swapchain_create_info{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = m_core->get_surface(),
      .minImageCount = m_image_count,
      .imageFormat = m_format.format,
      .imageColorSpace = m_format.colorSpace,
      .imageExtent = m_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = m_pre_transform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = m_present_mode,
      .clipped = VK_TRUE,
  };

  uint32_t queue_family_indices[] = {m_core->get_queue_family_index_graphics(),
                                     m_core->get_queue_family_index_present()};
  if (queue_family_indices[0] != queue_family_indices[1])
  {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
  }

  if (vkCreateSwapchainKHR(m_core->get_device(), &swapchain_create_info, nullptr, &m_swapchain) != VK_SUCCESS)
    return "Failed to create swapchain";

  m_images = vkEnumCall<VkImage>(vkGetSwapchainImagesKHR, m_core->get_device(), m_swapchain);

  // Note that we use the *actual* number of images here, which can be
  // more than the minImageCount we asked for.
  for (auto semaphore : m_render_done_semaphores)
    vkDestroySemaphore(m_core->get_device(), semaphore, nullptr);
  m_render_done_semaphores.assign(m_images.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_create_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (auto &semaphore : m_render_done_semaphores)
  {
    if (vkCreateSemaphore(m_core->get_device(), &semaphore_create_info, nullptr, &semaphore) != VK_SUCCESS)
      return "Failed to create a semaphore";
  }

  m_image_views.reserve(m_images.size());
  for (const auto &image : m_images)
  {
    VkImageViewCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_format.format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkImageView view{};
    if (vkCreateImageView(m_core->get_device(), &create_info, nullptr, &view) != VK_SUCCESS)
      return "Failed to create a swapchain image view";

    m_image_views.push_back(view);
  }

  {
    auto result = Image::create(m_core->get_device(), m_core->get_allocator(), VK_FORMAT_D32_SFLOAT, m_extent,
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if (result.is_error())
      return result.get_error();

    m_depth_image = std::move(result).take_value();

    PROPAGATE_RESULT_ERROR(m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
      if (!m_depth_image->transition_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL))
        return "Failed to transition depth image layout";

      return Ok();
    }));
  }

  m_is_out_of_date = false;

  return Ok();
}

Result<> Swapchain::create_frame_resources()
{
  VkCommandPoolCreateInfo commmand_pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = m_core->get_queue_family_index_graphics(),
  };
  if (vkCreateCommandPool(m_core->get_device(), &commmand_pool_create_info, nullptr, &m_frame_resource_command_pool) !=
      VK_SUCCESS)
    return "Failed to create command pool";

  VkCommandBufferAllocateInfo command_buffer_alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_frame_resource_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  for (uint32_t i = 0; i < NUM_CONCURRENT_FRAMES; i++)
  {
    if (vkAllocateCommandBuffers(m_core->get_device(), &command_buffer_alloc_info, &m_frames[i].cmd_buffer) !=
        VK_SUCCESS)
      return "Failed to allocate frame command buffer";

    VkSemaphoreCreateInfo semaphore_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    if (vkCreateSemaphore(m_core->get_device(), &semaphore_create_info, nullptr, &m_frames[i].sm_image_available) !=
        VK_SUCCESS)
      return "Failed to create a semaphore";

    VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    if (vkCreateFence(m_core->get_device(), &fence_create_info, nullptr, &m_frames[i].fen_in_flight) != VK_SUCCESS)
      return "Failed to create a fence";
  }

  return Ok();
}
