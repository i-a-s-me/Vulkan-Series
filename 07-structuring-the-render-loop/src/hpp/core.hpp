// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <common.hpp>

class VulkanCore
{
    static constexpr const char *VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
    static constexpr const char *SWAPCHAIN_DEVICE_EXTENSION_NAME = "VK_KHR_swapchain";
    static constexpr const char *PORTABILITY_SUBSET_DEVICE_EXTENSION_NAME = "VK_KHR_portability_subset";

public:
    static Result<Box<VulkanCore>> create(std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback);

    VulkanCore(const VulkanCore &) = delete;
    VulkanCore &operator=(const VulkanCore &) = delete;

    ~VulkanCore();

private:
    VulkanCore() = default;

    Result<> create_instance();
    Result<> select_physical_device();
    Result<> create_device();

private:
    VkInstance m_instance{};
    VkDevice m_device{};
    VkSurfaceKHR m_surface{};
    VmaAllocator m_allocator{};
    std::string m_physical_device_name{};
    VkPhysicalDevice m_physical_device{};
    VkDebugUtilsMessengerEXT m_debug_messenger{};

    VkQueue m_present_queue{};
    VkQueue m_graphics_queue{};
    uint32_t m_present_queue_family_index{};
    uint32_t m_graphics_queue_family_index{};

    VkFence m_single_shot_command_fence{};
    VkCommandPool m_single_shot_command_pool{};
    VkCommandBuffer m_single_shot_command_buffer{};
};