#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cassert>

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class SampleVulkan
{
  static constexpr uint32_t NUM_CONCURRENT_FRAMES = 3;
  static constexpr const char *VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
  static constexpr const char *SWAPCHAIN_DEVICE_EXTENSION_NAME = "VK_KHR_swapchain";
  static constexpr const char *PORTABILITY_SUBSET_DEVICE_EXTENSION_NAME = "VK_KHR_portability_subset";

  static constexpr uint8_t SHADER_SPV_VERT[] = {
  	0x03,0x02,0x23,0x07,0x00,0x00,0x01,0x00,0x0B,0x00,0x0D,0x00,0x28,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x11,0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x0B,0x00,0x06,0x00,
  	0x01,0x00,0x00,0x00,0x47,0x4C,0x53,0x4C,0x2E,0x73,0x74,0x64,0x2E,0x34,0x35,0x30,
  	0x00,0x00,0x00,0x00,0x0E,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
  	0x0F,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x6D,0x61,0x69,0x6E,
  	0x00,0x00,0x00,0x00,0x19,0x00,0x00,0x00,0x1D,0x00,0x00,0x00,0x03,0x00,0x03,0x00,
  	0x02,0x00,0x00,0x00,0xC2,0x01,0x00,0x00,0x04,0x00,0x0A,0x00,0x47,0x4C,0x5F,0x47,
  	0x4F,0x4F,0x47,0x4C,0x45,0x5F,0x63,0x70,0x70,0x5F,0x73,0x74,0x79,0x6C,0x65,0x5F,
  	0x6C,0x69,0x6E,0x65,0x5F,0x64,0x69,0x72,0x65,0x63,0x74,0x69,0x76,0x65,0x00,0x00,
  	0x04,0x00,0x08,0x00,0x47,0x4C,0x5F,0x47,0x4F,0x4F,0x47,0x4C,0x45,0x5F,0x69,0x6E,
  	0x63,0x6C,0x75,0x64,0x65,0x5F,0x64,0x69,0x72,0x65,0x63,0x74,0x69,0x76,0x65,0x00,
  	0x05,0x00,0x04,0x00,0x04,0x00,0x00,0x00,0x6D,0x61,0x69,0x6E,0x00,0x00,0x00,0x00,
  	0x05,0x00,0x05,0x00,0x0C,0x00,0x00,0x00,0x70,0x6F,0x73,0x69,0x74,0x69,0x6F,0x6E,
  	0x73,0x00,0x00,0x00,0x05,0x00,0x06,0x00,0x17,0x00,0x00,0x00,0x67,0x6C,0x5F,0x50,
  	0x65,0x72,0x56,0x65,0x72,0x74,0x65,0x78,0x00,0x00,0x00,0x00,0x06,0x00,0x06,0x00,
  	0x17,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x67,0x6C,0x5F,0x50,0x6F,0x73,0x69,0x74,
  	0x69,0x6F,0x6E,0x00,0x06,0x00,0x07,0x00,0x17,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
  	0x67,0x6C,0x5F,0x50,0x6F,0x69,0x6E,0x74,0x53,0x69,0x7A,0x65,0x00,0x00,0x00,0x00,
  	0x06,0x00,0x07,0x00,0x17,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x67,0x6C,0x5F,0x43,
  	0x6C,0x69,0x70,0x44,0x69,0x73,0x74,0x61,0x6E,0x63,0x65,0x00,0x06,0x00,0x07,0x00,
  	0x17,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x67,0x6C,0x5F,0x43,0x75,0x6C,0x6C,0x44,
  	0x69,0x73,0x74,0x61,0x6E,0x63,0x65,0x00,0x05,0x00,0x03,0x00,0x19,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x05,0x00,0x06,0x00,0x1D,0x00,0x00,0x00,0x67,0x6C,0x5F,0x56,
  	0x65,0x72,0x74,0x65,0x78,0x49,0x6E,0x64,0x65,0x78,0x00,0x00,0x47,0x00,0x03,0x00,
  	0x17,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x48,0x00,0x05,0x00,0x17,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x05,0x00,
  	0x17,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
  	0x48,0x00,0x05,0x00,0x17,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,
  	0x03,0x00,0x00,0x00,0x48,0x00,0x05,0x00,0x17,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
  	0x0B,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x47,0x00,0x04,0x00,0x1D,0x00,0x00,0x00,
  	0x0B,0x00,0x00,0x00,0x2A,0x00,0x00,0x00,0x13,0x00,0x02,0x00,0x02,0x00,0x00,0x00,
  	0x21,0x00,0x03,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x16,0x00,0x03,0x00,
  	0x06,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x17,0x00,0x04,0x00,0x07,0x00,0x00,0x00,
  	0x06,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x15,0x00,0x04,0x00,0x08,0x00,0x00,0x00,
  	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,0x08,0x00,0x00,0x00,
  	0x09,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x1C,0x00,0x04,0x00,0x0A,0x00,0x00,0x00,
  	0x07,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x20,0x00,0x04,0x00,0x0B,0x00,0x00,0x00,
  	0x06,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x3B,0x00,0x04,0x00,0x0B,0x00,0x00,0x00,
  	0x0C,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,0x06,0x00,0x00,0x00,
  	0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,0x06,0x00,0x00,0x00,
  	0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0xBF,0x2C,0x00,0x05,0x00,0x07,0x00,0x00,0x00,
  	0x0F,0x00,0x00,0x00,0x0D,0x00,0x00,0x00,0x0E,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,
  	0x06,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0x2C,0x00,0x05,0x00,
  	0x07,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,
  	0x2C,0x00,0x05,0x00,0x07,0x00,0x00,0x00,0x12,0x00,0x00,0x00,0x0E,0x00,0x00,0x00,
  	0x10,0x00,0x00,0x00,0x2C,0x00,0x06,0x00,0x0A,0x00,0x00,0x00,0x13,0x00,0x00,0x00,
  	0x0F,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x12,0x00,0x00,0x00,0x17,0x00,0x04,0x00,
  	0x14,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,
  	0x08,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x1C,0x00,0x04,0x00,
  	0x16,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x1E,0x00,0x06,0x00,
  	0x17,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x16,0x00,0x00,0x00,
  	0x16,0x00,0x00,0x00,0x20,0x00,0x04,0x00,0x18,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
  	0x17,0x00,0x00,0x00,0x3B,0x00,0x04,0x00,0x18,0x00,0x00,0x00,0x19,0x00,0x00,0x00,
  	0x03,0x00,0x00,0x00,0x15,0x00,0x04,0x00,0x1A,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
  	0x01,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,0x1A,0x00,0x00,0x00,0x1B,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x20,0x00,0x04,0x00,0x1C,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
  	0x1A,0x00,0x00,0x00,0x3B,0x00,0x04,0x00,0x1C,0x00,0x00,0x00,0x1D,0x00,0x00,0x00,
  	0x01,0x00,0x00,0x00,0x20,0x00,0x04,0x00,0x1F,0x00,0x00,0x00,0x06,0x00,0x00,0x00,
  	0x07,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,0x06,0x00,0x00,0x00,0x22,0x00,0x00,0x00,
  	0x00,0x00,0x80,0x3F,0x20,0x00,0x04,0x00,0x26,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
  	0x14,0x00,0x00,0x00,0x36,0x00,0x05,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xF8,0x00,0x02,0x00,0x05,0x00,0x00,0x00,
  	0x3E,0x00,0x03,0x00,0x0C,0x00,0x00,0x00,0x13,0x00,0x00,0x00,0x3D,0x00,0x04,0x00,
  	0x1A,0x00,0x00,0x00,0x1E,0x00,0x00,0x00,0x1D,0x00,0x00,0x00,0x41,0x00,0x05,0x00,
  	0x1F,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x1E,0x00,0x00,0x00,
  	0x3D,0x00,0x04,0x00,0x07,0x00,0x00,0x00,0x21,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
  	0x51,0x00,0x05,0x00,0x06,0x00,0x00,0x00,0x23,0x00,0x00,0x00,0x21,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x51,0x00,0x05,0x00,0x06,0x00,0x00,0x00,0x24,0x00,0x00,0x00,
  	0x21,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x50,0x00,0x07,0x00,0x14,0x00,0x00,0x00,
  	0x25,0x00,0x00,0x00,0x23,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x0D,0x00,0x00,0x00,
  	0x22,0x00,0x00,0x00,0x41,0x00,0x05,0x00,0x26,0x00,0x00,0x00,0x27,0x00,0x00,0x00,
  	0x19,0x00,0x00,0x00,0x1B,0x00,0x00,0x00,0x3E,0x00,0x03,0x00,0x27,0x00,0x00,0x00,
  	0x25,0x00,0x00,0x00,0xFD,0x00,0x01,0x00,0x38,0x00,0x01,0x00,
  };
  
  static constexpr uint8_t SHADER_SPV_FRAG[] = {
  	0x03,0x02,0x23,0x07,0x00,0x00,0x01,0x00,0x0B,0x00,0x0D,0x00,0x0D,0x00,0x00,0x00,
  	0x00,0x00,0x00,0x00,0x11,0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x0B,0x00,0x06,0x00,
  	0x01,0x00,0x00,0x00,0x47,0x4C,0x53,0x4C,0x2E,0x73,0x74,0x64,0x2E,0x34,0x35,0x30,
  	0x00,0x00,0x00,0x00,0x0E,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
  	0x0F,0x00,0x06,0x00,0x04,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x6D,0x61,0x69,0x6E,
  	0x00,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x10,0x00,0x03,0x00,0x04,0x00,0x00,0x00,
  	0x07,0x00,0x00,0x00,0x03,0x00,0x03,0x00,0x02,0x00,0x00,0x00,0xC2,0x01,0x00,0x00,
  	0x04,0x00,0x0A,0x00,0x47,0x4C,0x5F,0x47,0x4F,0x4F,0x47,0x4C,0x45,0x5F,0x63,0x70,
  	0x70,0x5F,0x73,0x74,0x79,0x6C,0x65,0x5F,0x6C,0x69,0x6E,0x65,0x5F,0x64,0x69,0x72,
  	0x65,0x63,0x74,0x69,0x76,0x65,0x00,0x00,0x04,0x00,0x08,0x00,0x47,0x4C,0x5F,0x47,
  	0x4F,0x4F,0x47,0x4C,0x45,0x5F,0x69,0x6E,0x63,0x6C,0x75,0x64,0x65,0x5F,0x64,0x69,
  	0x72,0x65,0x63,0x74,0x69,0x76,0x65,0x00,0x05,0x00,0x04,0x00,0x04,0x00,0x00,0x00,
  	0x6D,0x61,0x69,0x6E,0x00,0x00,0x00,0x00,0x05,0x00,0x05,0x00,0x09,0x00,0x00,0x00,
  	0x6F,0x75,0x74,0x43,0x6F,0x6C,0x6F,0x72,0x00,0x00,0x00,0x00,0x47,0x00,0x04,0x00,
  	0x09,0x00,0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x13,0x00,0x02,0x00,
  	0x02,0x00,0x00,0x00,0x21,0x00,0x03,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
  	0x16,0x00,0x03,0x00,0x06,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x17,0x00,0x04,0x00,
  	0x07,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x20,0x00,0x04,0x00,
  	0x08,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x3B,0x00,0x04,0x00,
  	0x08,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x2B,0x00,0x04,0x00,
  	0x06,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x80,0x3F,0x2B,0x00,0x04,0x00,
  	0x06,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2C,0x00,0x07,0x00,
  	0x07,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,
  	0x0B,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x36,0x00,0x05,0x00,0x02,0x00,0x00,0x00,
  	0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xF8,0x00,0x02,0x00,
  	0x05,0x00,0x00,0x00,0x3E,0x00,0x03,0x00,0x09,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,
  	0xFD,0x00,0x01,0x00,0x38,0x00,0x01,0x00,
  };

  struct FrameResourceSet
  {
    VkFence fen_in_flight{};
    VkSemaphore sm_image_available{};
    VkCommandBuffer cmd_buffer{};
  };

  public:

  SampleVulkan() = default;
  SampleVulkan(const SampleVulkan &) = delete;
  SampleVulkan &operator=(const SampleVulkan &) = delete;

  ~SampleVulkan();

  bool initialize(std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback);

  bool render();

  void resize(uint32_t fb_width, uint32_t fb_height);

  private:
  bool create_instance();
  bool select_physical_device();
  bool create_device_and_swapchain();
  bool create_frame_resources();

  bool create_graphics_pipeline();

  void draw_frame(VkCommandBuffer cmd, VkImageView color_attachment);

  bool recreate_swapchain();

  bool transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
                               uint32_t base_mip_level = 0, uint32_t level_count = 1, uint32_t base_array_layer = 0,
                               uint32_t layer_count = 1);

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

  VkDevice m_device{};
  VkQueue m_graphics_queue{};
  VkQueue m_present_queue{};
  VkSwapchainKHR m_swapchain{};
  std::vector<VkImage> m_swapchain_images{};
  std::vector<VkImageView> m_swapchain_image_views{};

  uint32_t m_current_frame_index{};
  VkCommandPool m_frame_resource_command_pool{};
  FrameResourceSet m_frames[NUM_CONCURRENT_FRAMES];

  VkPipeline m_graphics_pipeline{};
  VkPipelineLayout m_graphics_pipeline_layout{};
};

class SampleApp
{
  static constexpr uint32_t SCREEN_WIDTH = 800;
  static constexpr uint32_t SCREEN_HEIGHT = 600;

  public:
  int launch(int argc, char *argv[]);

  ~SampleApp();

  private:
  GLFWwindow *m_window{};
  std::unique_ptr<SampleVulkan> m_vulkan{};
};

int main(int argc, char *argv[])
{
  SampleApp app;
  return app.launch(argc, argv);
}

// -----------------------------------------------------------------------------

int SampleApp::launch(int argc, char *argv[])
{
  if (!glfwInit())
  {
    printf("Failed to initialize GLFW\n");
    return -1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "i-a-s.me Vulkan Sample", nullptr, nullptr);
  if (!m_window)
  {
    printf("Failed to create GLFW window\n");
    return -1;
  }

  m_vulkan = std::make_unique<SampleVulkan>();
  if (!m_vulkan->initialize([window = m_window](VkInstance instance, uint32_t &out_width, uint32_t &out_height,
                                                VkSurfaceKHR &out_surface) -> bool {
        if (glfwCreateWindowSurface(instance, window, nullptr, &out_surface) != VK_SUCCESS)
        {
          printf("Failed to create Vulkan surface\n");
          return false;
        }

        int32_t fb_width{}, fb_height{};
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        while ((fb_width == 0) || (fb_height == 0))
        {
          glfwWaitEvents();
          glfwGetFramebufferSize(window, &fb_width, &fb_height);
        }

        out_width = (uint32_t) fb_width;
        out_height = (uint32_t) fb_height;

        return true;
      }))
  {
    printf("Failed to initialize Vulkan\n");
    return -1;
  }

  glfwSetWindowUserPointer(m_window, m_vulkan.get());

  glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
    int32_t fb_width{}, fb_height{};
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    if ((fb_width == 0) || (fb_height == 0))
      return;
    static_cast<SampleVulkan *>(glfwGetWindowUserPointer(window))->resize(fb_width, fb_height);
  });

  while (!glfwWindowShouldClose(m_window))
  {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE))
      glfwSetWindowShouldClose(m_window, true);

    glfwPollEvents();

    // Don't render while the window is minimized (its framebuffer is 0x0 then)
    int32_t fb_width{}, fb_height{};
    glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
    if ((fb_width == 0) || (fb_height == 0))
    {
      glfwWaitEvents();
      continue;
    }

    if (!m_vulkan->render())
      break;
  }

  return 0;
}

SampleApp::~SampleApp()
{
  m_vulkan.reset();

  if (m_window)
    glfwDestroyWindow(m_window);

  glfwTerminate();
}

// -----------------------------------------------------------------------------

template<typename ElementT, typename FuncT, typename... Args> std::vector<ElementT> vkEnumCall(FuncT f, Args... args)
{
  uint32_t _count{};
  f(args..., &_count, nullptr);

  std::vector<ElementT> result(_count);
  f(args..., &_count, result.data());

  return result;
}

SampleVulkan::~SampleVulkan()
{
  if (m_device)
  {
    vkDeviceWaitIdle(m_device);

    vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_graphics_pipeline_layout, nullptr);

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

  if (!create_frame_resources())
    return false;

  if (!create_graphics_pipeline())
    return false;

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
                      0.2f,
                      0.4f,
                      0.8f,
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

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
  vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdEndRendering(cmd);
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

bool SampleVulkan::create_graphics_pipeline()
{
  VkPipelineLayoutCreateInfo layout_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pushConstantRangeCount = 0,
  };

  if (vkCreatePipelineLayout(m_device, &layout_create_info, nullptr, &m_graphics_pipeline_layout) != VK_SUCCESS) {
    printf("Failed to create the graphics pipeline layout\n");
    return false;
  }

  VkShaderModule shader_modules[2];

  VkShaderModuleCreateInfo shader_module_create_infos[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(SHADER_SPV_VERT),
      .pCode = (const uint32_t*)SHADER_SPV_VERT,
    },
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(SHADER_SPV_FRAG),
      .pCode = (const uint32_t*)SHADER_SPV_FRAG,
    },
  };
  
  if (vkCreateShaderModule(m_device, &shader_module_create_infos[0], nullptr, &shader_modules[0]) != VK_SUCCESS) {
    printf("Failed to create the shader module for vertex shader\n");
    return false;
  }

  if (vkCreateShaderModule(m_device, &shader_module_create_infos[1], nullptr, &shader_modules[1]) != VK_SUCCESS) {
    printf("Failed to create the shader module for fragment shader\n");
    
    vkDestroyShaderModule(m_device, shader_modules[0], nullptr);
    return false;
  }

  VkPipelineShaderStageCreateInfo shader_stage_create_infos[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = shader_modules[0],
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = shader_modules[1],
      .pName = "main",
    },
  };

  VkPipelineRenderingCreateInfo rendering_create_info {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &m_swapchain_format.format,
    .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 0,
    .vertexAttributeDescriptionCount = 0,
  };

  VkPipelineInputAssemblyStateCreateInfo ia_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo rs_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .lineWidth = 1.0,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment_state{
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_state,
  };

  VkDynamicState dynamic_states[2] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = (sizeof(dynamic_states)/sizeof(dynamic_states[0])),
    .pDynamicStates = dynamic_states,
  };

  VkGraphicsPipelineCreateInfo create_info{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &rendering_create_info,

    .stageCount = (sizeof(shader_stage_create_infos)/sizeof(shader_stage_create_infos[0])),
    .pStages = shader_stage_create_infos,

    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &ia_state_create_info,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rs_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDynamicState = &dynamic_state_create_info,

    .layout = m_graphics_pipeline_layout,
  };

  if (vkCreateGraphicsPipelines(m_device, nullptr, 1, &create_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS) {
    printf("Failed to create the graphics pipeline\n");

    vkDestroyShaderModule(m_device, shader_modules[0], nullptr);
    vkDestroyShaderModule(m_device, shader_modules[1], nullptr);
    return false;
  }

  vkDestroyShaderModule(m_device, shader_modules[0], nullptr);
  vkDestroyShaderModule(m_device, shader_modules[1], nullptr);

  return true;
}