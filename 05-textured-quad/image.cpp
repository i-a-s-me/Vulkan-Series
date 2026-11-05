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
    .initialLayout = image->m_layout,
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

bool Image::transition_layout(VkCommandBuffer cmd, VkImageLayout new_layout)
{
  if (transition_image_layout(cmd, m_handle, m_layout, new_layout)) {
    m_layout = new_layout;
    return true;
  }

  return false;
}

bool Image::transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout,
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
  else if (_is_layout_pair(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
    src_stage = VK_PIPELINE_STAGE_2_NONE;
    src_access = VK_ACCESS_2_NONE;

    dst_stage = VK_PIPELINE_STAGE_2_COPY_BIT;
    dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }
  else if (_is_layout_pair(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
    src_stage = VK_PIPELINE_STAGE_2_COPY_BIT;
    src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    dst_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    dst_access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
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
