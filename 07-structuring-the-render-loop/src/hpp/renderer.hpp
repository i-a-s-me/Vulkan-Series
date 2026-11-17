// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "image.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"

class Renderer
{
  static constexpr uint32_t NUM_CONCURRENT_FRAMES = 3;

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
  
  bool create_frame_resources();

  void draw_frame(VkCommandBuffer cmd, VkImageView color_attachment);

  bool execute_single_shot_commands(std::function<Result<>(VkCommandBuffer)> commands);

  bool recreate_swapchain();

private:
  uint32_t m_swapchain_image_count{};
  std::vector<VkSemaphore> m_render_done_semaphores;
  VkSurfaceFormatKHR m_swapchain_format{};
  VkPresentModeKHR m_swapchain_present_mode{};
  VkExtent2D m_swapchain_extent{1, 1};
  VkExtent2D m_pending_swapchain_extent{1, 1};
  bool m_swapchain_is_out_of_date{};
  VkSurfaceTransformFlagBitsKHR m_swapchain_pre_transform{};
  Box<Image> m_depth_attachment{};
  
  VkSwapchainKHR m_swapchain{};
  std::vector<VkImage> m_swapchain_images{};
  std::vector<VkImageView> m_swapchain_image_views{};

  uint32_t m_current_frame_index{};
  VkCommandPool m_frame_resource_command_pool{};
  FrameResourceSet m_frames[NUM_CONCURRENT_FRAMES];

  Box<Pipeline> m_graphics_pipeline{};

  Box<Buffer> m_staging_buffer{};
  Box<Buffer> m_vertex_buffer{};
  Box<Buffer> m_index_buffer{};
  Box<Buffer> m_vertex_uniform_buffer{};

  Box<Image> m_texture_image{};
  VkSampler m_texture_sampler{};
  
  Box<ShaderDataTable> m_shader_data_table{};
};
