// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include <image.hpp>

class VulkanCore;

class Swapchain
{
  struct FrameResourceSet
  {
    VkFence fen_in_flight{};
    VkSemaphore sm_image_available{};
    VkCommandBuffer cmd_buffer{};
  };

  public:
  static constexpr uint32_t NUM_CONCURRENT_FRAMES = 3;

  static Result<Box<Swapchain>> create(const VulkanCore *core, VkExtent2D initial_extent);

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  ~Swapchain();

  void resize(uint32_t width, uint32_t height);
  void request_recreation();

  Result<VkCommandBuffer> begin_frame();
  Result<> end_frame();

  public:
  void register_resize_callback(std::function<void(VkExtent2D extent)> resize_callback)
  {
    m_resize_callback = resize_callback;
  }

  VkFormat get_color_format() const
  {
    return m_format.format;
  }

  VkFormat get_depth_format() const
  {
    return m_depth_image->get_format();
  }

  VkExtent2D get_extent() const
  {
    return m_extent;
  }

  uint32_t get_current_frame_index() const
  {
    return m_current_frame_index;
  }

  VkImageView get_color_attachment() const
  {
    return m_image_views[m_current_image_index];
  }

  VkImageView get_depth_attachment() const
  {
    return m_depth_image->get_view();
  }

  float get_aspect_ratio() const
  {
    return (float) m_extent.width / (float) m_extent.height;
  }

  private:
  Swapchain(const VulkanCore *core) : m_core(core)
  {
  }

  Result<> recreate();
  Result<> create_frame_resources();

  private:
  const VulkanCore *const m_core;

  bool m_is_out_of_date{};

  uint32_t m_image_count{};
  VkSurfaceFormatKHR m_format{};
  VkPresentModeKHR m_present_mode{};
  std::vector<VkSemaphore> m_render_done_semaphores;
  VkSurfaceTransformFlagBitsKHR m_pre_transform{};

  VkSwapchainKHR m_swapchain{};
  std::vector<VkImage> m_images{};
  std::vector<VkImageView> m_image_views{};
  Box<Image> m_depth_image{};

  uint32_t m_current_frame_index{};
  uint32_t m_current_image_index{};
  VkCommandPool m_frame_resource_command_pool{};
  FrameResourceSet m_frames[NUM_CONCURRENT_FRAMES];

  VkExtent2D m_extent{1, 1};
  VkExtent2D m_pending_extent{1, 1};

  std::function<void(VkExtent2D extent)> m_resize_callback{};
};