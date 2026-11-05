// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "common.hpp"

class Image
{
public:
  static Result<Box<Image>> create(VkDevice device, VmaAllocator allocator, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage);

  Image(const Image&) = delete;
  ~Image();

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
};