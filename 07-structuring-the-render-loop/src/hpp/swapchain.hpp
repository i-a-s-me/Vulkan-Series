// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <common.hpp>

class VulkanCore;

class Swapchain
{
  static constexpr uint32_t NUM_CONCURRENT_FRAMES = 3;

  struct FrameResourceSet
  {
    VkFence fen_in_flight{};
    VkSemaphore sm_image_available{};
    VkCommandBuffer cmd_buffer{};
  };

  public:
  static Result<Box<Swapchain>> create(const VulkanCore *core);

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  ~Swapchain();

  private:
  Swapchain(const VulkanCore *core) : m_core(core)
  {
  }

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

  uint32_t m_current_frame_index{};
  VkCommandPool m_frame_resource_command_pool{};
  FrameResourceSet m_frames[NUM_CONCURRENT_FRAMES];

  VkExtent2D m_extent{1, 1};
  VkExtent2D m_pending_extent{1, 1};
};