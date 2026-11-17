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

  return swapchain;
}

Swapchain::~Swapchain()
{
  if (!m_core || !m_core->get_device())
    return;
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

  m_is_out_of_date = false;

  return Ok();
}
