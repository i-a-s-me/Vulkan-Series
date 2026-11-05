// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "image.hpp"

Result<Box<Image>> Image::create(VkDevice device, VmaAllocator allocator, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage)
{
  Box<Image> image(new Image(device, allocator));

  image->m_format = format;
  image->m_extent = extent;

  VkImageCreateInfo create_info{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D, // for now we'll only need 2D images
    .format = format,
    .extent = VkExtent3D(extent.width, extent.height, 1),
    .mipLevels = 1, // we won't be bothering with mip mapping for now
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT, // we won't be bothering with multisampling for now either
    .tiling = VK_IMAGE_TILING_OPTIMAL, // we don't have any specific tiling we need so let vulkan pick the optimal one
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // we only need to use our images from a single queue family at a time
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo alloc_create_info{
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  if (vmaCreateImage(allocator, &create_info, &alloc_create_info, &image->m_handle, &image->m_allocation, nullptr) != VK_SUCCESS)
    return "Failed to create the image";

  VkImageViewCreateInfo view_create_info{
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image->m_handle,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image->m_format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };
  if (vkCreateImageView(device, &view_create_info, nullptr, &image->m_view_handle) != VK_SUCCESS)
    return "Failed to create the image view";

  return image;
}

Image::~Image()
{
  if (m_allocator && m_allocation && m_handle)
    vmaDestroyImage(m_allocator, m_handle, m_allocation);

  if (m_device && m_view_handle)
    vkDestroyImageView(m_device, m_view_handle, nullptr);
}
