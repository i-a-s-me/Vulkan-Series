// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <core.hpp>

template<typename ElementT, typename FuncT, typename... Args> std::vector<ElementT> vkEnumCall(FuncT f, Args... args)
{
  uint32_t _count{};
  f(args..., &_count, nullptr);

  std::vector<ElementT> result(_count);
  f(args..., &_count, result.data());

  return result;
}

Result<Box<VulkanCore>> VulkanCore::create(
    std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback)
{
  Box<VulkanCore> core(new VulkanCore());

  PROPAGATE_RESULT_ERROR(core->create_instance());

  if (!surface_creation_callback(core->m_instance, core->m_pending_swapchain_extent.width,
                                 core->m_pending_swapchain_extent.height, core->m_surface))
    return "Failed to create Vulkan surface";

  PROPAGATE_RESULT_ERROR(core->select_physical_device());
  PROPAGATE_RESULT_ERROR(core->create_device());
  PROPAGATE_RESULT_ERROR(core->create_allocator());
  PROPAGATE_RESULT_ERROR(core->initialize_single_shot_commands());

  return core;
}

VulkanCore::~VulkanCore()
{
  if (m_device)
  {
    vkDeviceWaitIdle(m_device);

    vkDestroyFence(m_device, m_single_shot_command_fence, nullptr);

    vkDestroyCommandPool(m_device, m_single_shot_command_pool, nullptr);

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

Result<> VulkanCore::create_instance()
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
    return "Failed to create Vulkan instance";

  const auto debug_messenger_create_func =
      (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
  if (enable_validation_layers && debug_messenger_create_func)
  {
    if (debug_messenger_create_func(m_instance, &debug_messenger_create_info, nullptr, &m_debug_messenger) !=
        VK_SUCCESS)
      return "Failed to create Vulkan debug messenger";
  }

  return Ok();
}

Result<> VulkanCore::select_physical_device()
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
    return "Failed to find a good physical device";

  printf("[VulkanCore]: Using physical device \"%s\"\n", m_physical_device_name.c_str());

  return Ok();
}

Result<> VulkanCore::create_device()
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
    return "Failed to create Vulkan device";

  vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue);
  vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);

  return Ok();
}

Result<> VulkanCore::create_allocator()
{
  VmaAllocatorCreateInfo vma_create_info{
      .physicalDevice = m_physical_device,
      .device = m_device,
      .instance = m_instance,
      .vulkanApiVersion = VK_API_VERSION_1_3,
  };
  if (vmaCreateAllocator(&vma_create_info, &m_allocator) != VK_SUCCESS)
    return "Failed to create the VMA allocator";

  return Ok();
}

Result<> VulkanCore::initialize_single_shot_commands()
{
  VkCommandPoolCreateInfo pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = m_graphics_queue_family_index,
  };
  if (vkCreateCommandPool(m_device, &pool_create_info, nullptr, &m_single_shot_command_pool) != VK_SUCCESS)
    return "Failed to create the single shot command pool";

  VkFenceCreateInfo fence_create_info{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
  };
  if (vkCreateFence(m_device, &fence_create_info, nullptr, &m_single_shot_command_fence) != VK_SUCCESS)
    return "Failed to create the single shot fence";

  VkCommandBufferAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_single_shot_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  if (vkAllocateCommandBuffers(m_device, &alloc_info, &m_single_shot_command_buffer) != VK_SUCCESS)
    return "Failed to allocate the single shot command buffer";

  return Ok();
}
