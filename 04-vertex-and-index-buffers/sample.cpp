// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "sample.hpp"

#include <set>
#include <stdio.h>
#include <string.h>
#include <utility>
#include <algorithm>

#define CHECK_RESULT(_result, _var)             \
{                                               \
  auto result = _result;                        \
  if (result.is_error()) {                      \
    printf("%s\n", result.get_error().c_str()); \
    return false;                               \
  }                                             \
  _var = std::move(result).take_value();        \
}

template<typename ElementT, typename FuncT, typename... Args> std::vector<ElementT> vkEnumCall(FuncT f, Args... args)
{
  uint32_t _count{};
  f(args..., &_count, nullptr);

  std::vector<ElementT> result(_count);
  f(args..., &_count, result.data());

  return result;
}

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 colour;
};

static const Vertex VERTICES[4] = {
  {glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
  {glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
  {glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
  {glm::vec3(-0.5f, 0.5f, 0.0f),  glm::vec3(1.0f, 0.0f, 1.0f)},
};

static const uint32_t INDICES[6] = {
  0, 1, 2,
  2, 3, 0
};

SampleVulkan::~SampleVulkan()
{
  if (m_device)
  {
    vkDeviceWaitIdle(m_device);

    vkDestroyFence(m_device, m_single_shot_command_fence, nullptr);

    vkDestroyCommandPool(m_device, m_single_shot_command_pool, nullptr);

    m_vertex_buffer.reset();
    m_index_buffer.reset();
    m_staging_buffer.reset();

    m_graphics_pipeline.reset();

    for (uint32_t i = 0; i < NUM_CONCURRENT_FRAMES; i++)
    {
      vkDestroyFence(m_device, m_frames[i].fen_in_flight, nullptr);
      vkDestroySemaphore(m_device, m_frames[i].sm_image_available, nullptr);
    }

    for (size_t i = 0; i < m_render_done_semaphores.size(); i++)
      vkDestroySemaphore(m_device, m_render_done_semaphores[i], nullptr);
    m_render_done_semaphores.clear();

    vkDestroyCommandPool(m_device, m_frame_resource_command_pool, nullptr);

    for (const auto &view : m_swapchain_image_views)
      vkDestroyImageView(m_device, view, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    vmaDestroyAllocator(m_allocator);

    vkDestroyDevice(m_device, nullptr);
  }

  if (m_instance)
  {
    if (m_surface)
      vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    {
      const auto debug_messenger_destroy_func =
          (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
      if (debug_messenger_destroy_func && m_debug_messenger)
        debug_messenger_destroy_func(m_instance, m_debug_messenger, nullptr);
    }

    vkDestroyInstance(m_instance, nullptr);
  }
}

bool SampleVulkan::initialize(
    std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback)
{
  if (!create_instance())
    return false;

  if (!surface_creation_callback(m_instance, m_pending_swapchain_extent.width, m_pending_swapchain_extent.height,
                                 m_surface))
  {
    printf("Failed to create Vulkan surface\n");
    return false;
  }

  if (!select_physical_device())
    return false;

  if (!create_device_and_swapchain())
    return false;

  VmaAllocatorCreateInfo vma_create_info{
    .physicalDevice = m_physical_device,
    .device = m_device,
    .instance = m_instance,
    .vulkanApiVersion = VK_API_VERSION_1_3,
  };
  if (vmaCreateAllocator(&vma_create_info, &m_allocator) != VK_SUCCESS) {
    printf("Failed to create the VMA allocator\n");
    return false;
  }

  if (!create_frame_resources())
    return false;

  {
    VkCommandPoolCreateInfo pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = m_graphics_queue_family_index,
    };
    vkCreateCommandPool(m_device, &pool_create_info, nullptr, &m_single_shot_command_pool);

    VkFenceCreateInfo fence_create_info{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    vkCreateFence(m_device, &fence_create_info, nullptr, &m_single_shot_command_fence);

    VkCommandBufferAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_single_shot_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    vkAllocateCommandBuffers(m_device, &alloc_info, &m_single_shot_command_buffer);
  }

  CHECK_RESULT(
    Pipeline::create(
      m_device,
      m_swapchain_format.format,
      std::initializer_list<VkVertexInputBindingDescription>{
        {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
      },
      std::initializer_list<VkVertexInputAttributeDescription>{
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, colour)},
      },
      SHADER_SPV_VERT,
      SHADER_SPV_FRAG
    ),
    m_graphics_pipeline
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      std::max(sizeof(VERTICES), sizeof(INDICES)), // maximum size we'll need for our staging operations 
      Buffer::EUsage::STAGING
    ),
    m_staging_buffer
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      sizeof(VERTICES), 
      Buffer::EUsage::VERTEX
    ),
    m_vertex_buffer
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      sizeof(INDICES), 
      Buffer::EUsage::INDEX
    ),
    m_index_buffer
  );

  { // copy VERTICES to vertex buffer
    memcpy(m_staging_buffer->get_mapped_ptr(), VERTICES, sizeof(VERTICES));

    execute_single_shot_commands([&](VkCommandBuffer cmd)
    {
      VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = m_vertex_buffer->get_size(),
      };

      vkCmdCopyBuffer(cmd, m_staging_buffer->get_handle(), m_vertex_buffer->get_handle(), 1, &copy_region);
    });
  }

  { // copy INDICES to index buffer
    memcpy(m_staging_buffer->get_mapped_ptr(), INDICES, sizeof(INDICES));

    execute_single_shot_commands([&](VkCommandBuffer cmd)
    {
      VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = m_index_buffer->get_size(),
      };
      
      vkCmdCopyBuffer(cmd, m_staging_buffer->get_handle(), m_index_buffer->get_handle(), 1, &copy_region);
    });
  }

  return true;
}

bool SampleVulkan::render()
{
  const auto &current_frame = m_frames[m_current_frame_index];

  vkWaitForFences(m_device, 1, &current_frame.fen_in_flight, VK_TRUE, UINT64_MAX);

  uint32_t image_index{};
  switch (vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, current_frame.sm_image_available, VK_NULL_HANDLE,
                                &image_index))
  {
  case VK_ERROR_OUT_OF_DATE_KHR:
    if (!recreate_swapchain())
      return false;
    return true;

  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    break;

  default:
    printf("Failed to acquire swapchain image\n");
    return false;
  }

  VkCommandBufferBeginInfo cmd_begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkResetCommandBuffer(current_frame.cmd_buffer, 0);
  if (vkBeginCommandBuffer(current_frame.cmd_buffer, &cmd_begin_info) != VK_SUCCESS)
  {
    printf("Failed to start recording command buffer\n");
    return false;
  }

  if (!transition_image_layout(current_frame.cmd_buffer, m_swapchain_images[image_index], VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
  {
    printf("Failed to transition swapchain image\n");
    return false;
  }

  draw_frame(current_frame.cmd_buffer, m_swapchain_image_views[image_index]);

  if (!transition_image_layout(current_frame.cmd_buffer, m_swapchain_images[image_index],
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
  {
    printf("Failed to transition swapchain image\n");
    return false;
  }

  if (vkEndCommandBuffer(current_frame.cmd_buffer) != VK_SUCCESS)
  {
    printf("Failed to finish recording command buffer\n");
    return false;
  }

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

  vkResetFences(m_device, 1, &current_frame.fen_in_flight);
  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, current_frame.fen_in_flight) != VK_SUCCESS)
  {
    printf("Failed to submit frame command buffer\n");
    return false;
  }

  VkPresentInfoKHR present_info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_render_done_semaphores[image_index],
      .swapchainCount = 1,
      .pSwapchains = &m_swapchain,
      .pImageIndices = &image_index,
  };

  switch (vkQueuePresentKHR(m_present_queue, &present_info))
  {
  case VK_ERROR_OUT_OF_DATE_KHR:
  case VK_SUBOPTIMAL_KHR:
    if (!recreate_swapchain())
      return false;
    break;

  case VK_SUCCESS:
    break;

  default:
    printf("Failed to present swapchain\n");
    return false;
  }

  if (m_swapchain_is_out_of_date && !recreate_swapchain())
    return false;

  m_current_frame_index = (m_current_frame_index + 1) % NUM_CONCURRENT_FRAMES;

  return true;
}

void SampleVulkan::draw_frame(VkCommandBuffer cmd, VkImageView color_attachment)
{
  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = color_attachment,
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

  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea =
          {
              {},
              m_swapchain_extent,
          },
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
  };

  vkCmdBeginRendering(cmd, &rendering_info);

  VkViewport viewport = {
    .width = (float)m_swapchain_extent.width,
    .height = (float)m_swapchain_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{
    .extent = m_swapchain_extent,
  };
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  m_graphics_pipeline->bind(cmd);

  m_vertex_buffer->bind(cmd);
  m_index_buffer->bind(cmd);

  vkCmdDrawIndexed(cmd, sizeof(INDICES)/sizeof(INDICES[0]), 1, 0, 0, 0);

  vkCmdEndRendering(cmd);
}

void SampleVulkan::execute_single_shot_commands(std::function<void(VkCommandBuffer)> commands)
{
  vkResetCommandBuffer(m_single_shot_command_buffer, 0);

  VkCommandBufferBeginInfo begin_info{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkBeginCommandBuffer(m_single_shot_command_buffer, &begin_info);

  commands(m_single_shot_command_buffer);

  vkEndCommandBuffer(m_single_shot_command_buffer);

  VkSubmitInfo submit_info{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &m_single_shot_command_buffer,
  };
  vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_single_shot_command_fence);

  vkWaitForFences(m_device, 1, &m_single_shot_command_fence, true, UINT64_MAX);
  vkResetFences(m_device, 1, &m_single_shot_command_fence);
}

bool SampleVulkan::transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout,
                                           VkImageLayout new_layout, uint32_t base_mip_level, uint32_t level_count,
                                           uint32_t base_array_layer, uint32_t layer_count)
{
  VkPipelineStageFlags2 src_stage{}, dst_stage{};
  VkAccessFlags2 src_access{}, dst_access{};
  VkImageAspectFlags aspect{VK_IMAGE_ASPECT_COLOR_BIT};

#define _is_layout_pair(o, n) ((old_layout == o) && (new_layout == n))

  if (_is_layout_pair(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
  {
    src_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    src_access = VK_ACCESS_2_NONE;

    dst_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  }
  else if (_is_layout_pair(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL))
  {
    // Wait for any depth writes from a previous use of this image (e.g. the previous frame)
    src_stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    src_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dst_stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    dst_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  else if (_is_layout_pair(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
  {
    src_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    dst_stage = VK_PIPELINE_STAGE_2_NONE;
    dst_access = VK_ACCESS_2_NONE;
  }
  else
  {
    printf("Unknown layout transition\n");
    return false;
  }

#undef _is_layout_pair

  VkImageMemoryBarrier2 barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

      .srcStageMask = src_stage,
      .srcAccessMask = src_access,

      .dstStageMask = dst_stage,
      .dstAccessMask = dst_access,

      .oldLayout = old_layout,
      .newLayout = new_layout,

      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

      .image = image,

      .subresourceRange =
          {
              .aspectMask = aspect,
              .baseMipLevel = base_mip_level,
              .levelCount = level_count,
              .baseArrayLayer = base_array_layer,
              .layerCount = layer_count,
          },
  };

  VkDependencyInfo dep_info{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrier,
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);

  return true;
}

bool SampleVulkan::create_instance()
{
  // This is a sample, not production code, so we always enable validation layers,
  // unless they're unavailable
  const auto instance_layer_props = vkEnumCall<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
  const bool enable_validation_layers =
      std::any_of(instance_layer_props.begin(), instance_layer_props.end(),
                  [](VkLayerProperties p) { return strcmp(p.layerName, VALIDATION_LAYER_NAME) == 0; });

  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                            const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data) -> VkBool32 {
        printf("[VK_VALIDATION_LAYER]: %s\n", callback_data->pMessage);
        return VK_FALSE;
      },
  };

  std::vector<const char *> extensions;
  { // GLFW Extensions
    uint32_t glfw_ext_count{};
    const auto glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    extensions.insert(extensions.end(), glfw_ext, glfw_ext + glfw_ext_count);
  }
  if (enable_validation_layers)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // macOS support: MoltenVK is a "portability" implementation (it doesn't support all
  // of Vulkan), so the loader only exposes it to us if we explicitly opt in.
  const auto instance_ext_props = vkEnumCall<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
  const bool enable_portability =
      std::any_of(instance_ext_props.begin(), instance_ext_props.end(), [](VkExtensionProperties p) {
        return strcmp(p.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0;
      });
  if (enable_portability)
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

  VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "i-a-s.me Vulkan 1.3 Sample",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "i-a-s.me Vulkan 1.3 Sample",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };
  VkInstanceCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = enable_validation_layers ? &debug_messenger_create_info : nullptr,
      .flags = enable_portability ? (VkInstanceCreateFlags) VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0u,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = enable_validation_layers ? 1u : 0u,
      .ppEnabledLayerNames = enable_validation_layers ? &VALIDATION_LAYER_NAME : nullptr,
      .enabledExtensionCount = (uint32_t) extensions.size(),
      .ppEnabledExtensionNames = extensions.data(),
  };

  if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
  {
    printf("Failed to create Vulkan instance\n");
    return false;
  }

  const auto debug_messenger_create_func =
      (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
  if (enable_validation_layers && debug_messenger_create_func)
  {
    if (debug_messenger_create_func(m_instance, &debug_messenger_create_info, nullptr, &m_debug_messenger) !=
        VK_SUCCESS)
    {
      printf("Failed to create Vulkan debug messenger\n");
      return false;
    }
  }

  return true;
}

bool SampleVulkan::select_physical_device()
{
  const auto find_queue_families = [&](VkPhysicalDevice p) -> std::pair<uint32_t, uint32_t> {
    uint32_t graphics_family_index{UINT32_MAX}, present_family_index{UINT32_MAX};

    const auto queue_families = vkEnumCall<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, p);
    for (uint32_t i = 0; i < (uint32_t) queue_families.size(); i++)
    {
      const auto &f = queue_families[i];

      if (f.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        graphics_family_index = i;

      VkBool32 has_present_support{};
      if ((vkGetPhysicalDeviceSurfaceSupportKHR(p, i, m_surface, &has_present_support) == VK_SUCCESS) &&
          has_present_support)
        present_family_index = i;

      if ((graphics_family_index != UINT32_MAX) && (present_family_index != UINT32_MAX))
        break;
    }

    return {graphics_family_index, present_family_index};
  };

  m_physical_device = VK_NULL_HANDLE;

  const auto physical_devices = vkEnumCall<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_instance);

  for (const auto &p : physical_devices)
  {
    VkPhysicalDeviceProperties2 props{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    };
    vkGetPhysicalDeviceProperties2(p, &props);

    if (props.properties.apiVersion < VK_API_VERSION_1_3)
      continue; // If this physical device does not support at least Vulkan 1.3, we cannot use it. We use Vulkan 1.3
                // core features in this sample.

    const auto queue_family_indices = find_queue_families(p);

    if ((queue_family_indices.first == UINT32_MAX) || (queue_family_indices.second == UINT32_MAX))
      continue; // This physical device isn't what we want, if it lacks either graphics or present queue families

    // We could (and should) do more suitability checks in a real application,
    // but for this sample, this is just ok.

    m_physical_device = p;
    m_graphics_queue_family_index = queue_family_indices.first;
    m_present_queue_family_index = queue_family_indices.second;

    m_physical_device_name = props.properties.deviceName;

    if (props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      break; // If we find a discrete GPU, then prefer it
  }

  if (m_physical_device == VK_NULL_HANDLE)
  {
    printf("Failed to find a good physical device\n");
    return false;
  }

  printf("Using physical device \"%s\"\n", m_physical_device_name.c_str());

  return true;
}

void SampleVulkan::resize(uint32_t fb_width, uint32_t fb_height)
{
  m_pending_swapchain_extent = {fb_width, fb_height};
  m_swapchain_is_out_of_date = true;
}

bool SampleVulkan::create_device_and_swapchain()
{
  const float priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  // passing through std::set effectively dedups family indices.
  // not really necessary to use std::set here as we only have to consider
  // 2 families (graphics and present), so we could have just checked (family_index_1 == family_index_2),
  // but this std::set approach generalizes to higher orders (imagine doing if/else checks for combinations of 3 or 4)
  for (const auto index : std::set<uint32_t>{m_graphics_queue_family_index, m_present_queue_family_index})
    queue_create_infos.push_back(VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = index,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    });

  std::vector<const char *> device_extensions{SWAPCHAIN_DEVICE_EXTENSION_NAME};
  // macOS support: if the device is a portability implementation (i.e. MoltenVK),
  // it exposes VK_KHR_portability_subset, and the spec requires us to enable it.
  const auto device_ext_props =
      vkEnumCall<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, m_physical_device, nullptr);
  if (std::any_of(device_ext_props.begin(), device_ext_props.end(), [](VkExtensionProperties p) {
        return strcmp(p.extensionName, PORTABILITY_SUBSET_DEVICE_EXTENSION_NAME) == 0;
      }))
    device_extensions.push_back(PORTABILITY_SUBSET_DEVICE_EXTENSION_NAME);

  VkPhysicalDeviceFeatures enabled_features{};
  VkPhysicalDeviceVulkan13Features vk_13_features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };

  VkDeviceCreateInfo device_create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &vk_13_features,
      .queueCreateInfoCount = (uint32_t) queue_create_infos.size(),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = (uint32_t) device_extensions.size(),
      .ppEnabledExtensionNames = device_extensions.data(),
      .pEnabledFeatures = &enabled_features,
  };

  if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS)
  {
    printf("Failed to create Vulkan device\n");
    return false;
  }

  vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue);
  vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);

  const auto surface_formats =
      vkEnumCall<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, m_physical_device, m_surface);
  const auto surface_present_modes =
      vkEnumCall<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, m_physical_device, m_surface);
  assert(surface_formats.size());
  assert(surface_present_modes.size());
  m_swapchain_format = surface_formats[0];
  m_swapchain_present_mode = (std::find(surface_present_modes.begin(), surface_present_modes.end(),
                                        VK_PRESENT_MODE_MAILBOX_KHR) != surface_present_modes.end())
                                 ? VK_PRESENT_MODE_MAILBOX_KHR
                                 : VK_PRESENT_MODE_FIFO_KHR;

  return recreate_swapchain();
}

bool SampleVulkan::create_frame_resources()
{
  VkCommandPoolCreateInfo commmand_pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = m_graphics_queue_family_index,
  };
  if (vkCreateCommandPool(m_device, &commmand_pool_create_info, nullptr, &m_frame_resource_command_pool) != VK_SUCCESS)
  {
    printf("Failed to create command pool\n");
    return false;
  }

  VkCommandBufferAllocateInfo command_buffer_alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_frame_resource_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  for (uint32_t i = 0; i < NUM_CONCURRENT_FRAMES; i++)
  {
    if (vkAllocateCommandBuffers(m_device, &command_buffer_alloc_info, &m_frames[i].cmd_buffer) != VK_SUCCESS)
    {
      printf("Failed to allocate frame command buffer\n");
      return false;
    }

    VkSemaphoreCreateInfo semaphore_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    if (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].sm_image_available) != VK_SUCCESS)
    {
      printf("Failed to create a semaphore\n");
      return false;
    }

    VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    if (vkCreateFence(m_device, &fence_create_info, nullptr, &m_frames[i].fen_in_flight) != VK_SUCCESS)
    {
      printf("Failed to create a fence\n");
      return false;
    }
  }

  return true;
}

bool SampleVulkan::recreate_swapchain()
{
  vkDeviceWaitIdle(m_device);

  VkSurfaceCapabilitiesKHR surface_caps{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surface_caps) != VK_SUCCESS)
  {
    printf("Failed to query physical device surface caps\n");
    return false;
  }

  // A minimized window has a 0x0 surface, and a 0x0 swapchain is not allowed. So we
  // keep the current swapchain, and try again later (once the window is restored).
  if ((surface_caps.maxImageExtent.width == 0) || (surface_caps.maxImageExtent.height == 0))
  {
    m_swapchain_is_out_of_date = true;
    return true;
  }

  m_swapchain_image_count = std::min(surface_caps.minImageCount + 1,
                                     (surface_caps.maxImageCount == 0) ? UINT32_MAX : surface_caps.maxImageCount);

  m_swapchain_pre_transform = surface_caps.currentTransform;

  m_pending_swapchain_extent.width = std::clamp(m_pending_swapchain_extent.width, surface_caps.minImageExtent.width,
                                                surface_caps.maxImageExtent.width);
  m_pending_swapchain_extent.height = std::clamp(m_pending_swapchain_extent.height, surface_caps.minImageExtent.height,
                                                 surface_caps.maxImageExtent.height);
  m_swapchain_extent = m_pending_swapchain_extent;

  for (const auto &view : m_swapchain_image_views)
    vkDestroyImageView(m_device, view, nullptr);
  if (m_swapchain)
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
  m_swapchain_image_views.clear();
  m_swapchain = VK_NULL_HANDLE;

  VkSwapchainCreateInfoKHR swapchain_create_info{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = m_surface,
      .minImageCount = m_swapchain_image_count,
      .imageFormat = m_swapchain_format.format,
      .imageColorSpace = m_swapchain_format.colorSpace,
      .imageExtent = m_swapchain_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = m_swapchain_pre_transform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = m_swapchain_present_mode,
      .clipped = VK_TRUE,
  };

  uint32_t queue_family_indices[] = {m_graphics_queue_family_index, m_present_queue_family_index};
  if (m_graphics_queue_family_index != m_present_queue_family_index)
  {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
  }

  if (vkCreateSwapchainKHR(m_device, &swapchain_create_info, nullptr, &m_swapchain) != VK_SUCCESS)
  {
    printf("Failed to create swapchain\n");
    return false;
  }

  m_swapchain_images = vkEnumCall<VkImage>(vkGetSwapchainImagesKHR, m_device, m_swapchain);

  // Note that we use the *actual* number of images here, which can be
  // more than the minImageCount we asked for.
  for (auto semaphore : m_render_done_semaphores)
    vkDestroySemaphore(m_device, semaphore, nullptr);
  m_render_done_semaphores.assign(m_swapchain_images.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_create_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (auto &semaphore : m_render_done_semaphores)
  {
    if (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &semaphore) != VK_SUCCESS)
    {
      printf("Failed to create a semaphore\n");
      return false;
    }
  }

  m_swapchain_image_views.reserve(m_swapchain_images.size());
  for (const auto &image : m_swapchain_images)
  {
    VkImageViewCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_swapchain_format.format,
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
    if (vkCreateImageView(m_device, &create_info, nullptr, &view) != VK_SUCCESS)
    {
      printf("Failed to create a swapchain image view\n");
      return false;
    }

    m_swapchain_image_views.push_back(view);
  }

  m_swapchain_is_out_of_date = false;

  return true;
}
