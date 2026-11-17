// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "image.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"

#include <vector>
#include <string>
#include <cassert>
#include <stdint.h>
#include <functional>

class Renderer
{
  static constexpr uint32_t NUM_CONCURRENT_FRAMES = 3;
  static constexpr const char *VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
  static constexpr const char *SWAPCHAIN_DEVICE_EXTENSION_NAME = "VK_KHR_swapchain";
  static constexpr const char *PORTABILITY_SUBSET_DEVICE_EXTENSION_NAME = "VK_KHR_portability_subset";

  struct FrameResourceSet
  {
    VkFence fen_in_flight{};
    VkSemaphore sm_image_available{};
    VkCommandBuffer cmd_buffer{};
  };

public:
  Renderer() = default;
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  ~Renderer();

  bool initialize(std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback);

  bool render();

  void resize(uint32_t fb_width, uint32_t fb_height);

private:
  bool create_instance();
  bool select_physical_device();
  bool create_device_and_swapchain();
  bool create_frame_resources();

  void draw_frame(VkCommandBuffer cmd, VkImageView color_attachment);

  bool execute_single_shot_commands(std::function<Result<>(VkCommandBuffer)> commands);

  bool recreate_swapchain();

private:
  VkInstance m_instance{};
  VkDebugUtilsMessengerEXT m_debug_messenger{};

  VkSurfaceKHR m_surface{};
  std::string m_physical_device_name{};
  VkPhysicalDevice m_physical_device{};
  uint32_t m_graphics_queue_family_index{};
  uint32_t m_present_queue_family_index{};
  uint32_t m_swapchain_image_count{};
  std::vector<VkSemaphore> m_render_done_semaphores;
  VkSurfaceFormatKHR m_swapchain_format{};
  VkPresentModeKHR m_swapchain_present_mode{};
  VkExtent2D m_swapchain_extent{1, 1};
  VkExtent2D m_pending_swapchain_extent{1, 1};
  bool m_swapchain_is_out_of_date{};
  VkSurfaceTransformFlagBitsKHR m_swapchain_pre_transform{};
  Box<Image> m_depth_attachment{};

  VkDevice m_device{};
  VkQueue m_graphics_queue{};
  VkQueue m_present_queue{};
  VkSwapchainKHR m_swapchain{};
  std::vector<VkImage> m_swapchain_images{};
  std::vector<VkImageView> m_swapchain_image_views{};

  uint32_t m_current_frame_index{};
  VkCommandPool m_frame_resource_command_pool{};
  FrameResourceSet m_frames[NUM_CONCURRENT_FRAMES];

  Box<Pipeline> m_graphics_pipeline{};

  VmaAllocator m_allocator{};

  Box<Buffer> m_staging_buffer{};
  Box<Buffer> m_vertex_buffer{};
  Box<Buffer> m_index_buffer{};
  Box<Buffer> m_vertex_uniform_buffer{};

  VkFence m_single_shot_command_fence{};
  VkCommandPool m_single_shot_command_pool{};
  VkCommandBuffer m_single_shot_command_buffer{};

  Box<Image> m_texture_image{};
  VkSampler m_texture_sampler{};
  
  Box<ShaderDataTable> m_shader_data_table{};
};
