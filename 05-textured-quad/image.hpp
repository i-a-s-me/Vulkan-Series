// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "common.hpp"

class Image
{
public:
  static bool transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
        uint32_t base_mip_level = 0, uint32_t level_count = 1, uint32_t base_array_layer = 0,
        uint32_t layer_count = 1);

public:
  static Result<Box<Image>> create(VkDevice device, VmaAllocator allocator, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage);

  Image(const Image&) = delete;
  ~Image();

  bool transition_layout(VkCommandBuffer cmd, VkImageLayout new_layout);

private:
  Image(VkDevice device, VmaAllocator allocator): m_device(device), m_allocator(allocator) {}

private:
  VkDevice m_device{};
  VmaAllocator m_allocator{};

  VkImage m_handle{};
  VkImageView m_view_handle{};
  VkFormat m_format{};
  VkExtent2D m_extent{};
  VmaAllocation m_allocation{};
  VkImageLayout m_layout{VK_IMAGE_LAYOUT_UNDEFINED};
};