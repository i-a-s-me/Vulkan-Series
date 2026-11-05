// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "buffer.hpp"

Result<Box<Buffer>> Buffer::create(VmaAllocator allocator, VkDeviceSize size, EUsage usage)
{
  Box<Buffer> buffer{new Buffer(allocator)};

  buffer->m_size = size;
  buffer->m_usage = usage;

  VkBufferUsageFlags _usage{};
  VmaAllocationCreateFlags _mem_flags{};

  switch (usage) {
    case EUsage::VERTEX:
      // vertex buffer + transfer dst (for copying data in from staging buffer)
      _usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      break;

    case EUsage::INDEX:
      // index buffer + transfer dst (for copying data in from staging buffer)
      _usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      break;

    case EUsage::STAGING:
      // allows transferring data out from this buffer (for copying)
      _usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

      // allow writing to this buffer from CPU memory
      _mem_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
      break;
  }

  VkBufferCreateInfo create_info{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = _usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // we only need to use our buffers from a single queue family at a time
  };

  VmaAllocationCreateInfo alloc_create_info{
    .flags = _mem_flags,
    .usage = VMA_MEMORY_USAGE_AUTO, // when _mem_flags doesn't contain CPU access flags (like we set in above switch STAGING case), this picks fast GPU VRAM
  };

  VmaAllocationInfo alloc_info{};
  if (vmaCreateBuffer(allocator, &create_info, &alloc_create_info, &buffer->m_handle, &buffer->m_allocation, &alloc_info) != VK_SUCCESS) {
    return "Failed to create the buffer";
  }

  if (usage == EUsage::STAGING) {
    buffer->m_mapped_ptr = alloc_info.pMappedData;
  }

  return buffer;
}

Buffer::~Buffer()
{
  if (m_allocator && m_handle && m_allocation) 
    vmaDestroyBuffer(m_allocator, m_handle, m_allocation);
}

void Buffer::bind(VkCommandBuffer cmd)
{
  switch (m_usage) {
    case EUsage::VERTEX:
    {
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &m_handle, &offset);
    }
    break;

    case EUsage::INDEX:
      vkCmdBindIndexBuffer(cmd, m_handle, 0, VK_INDEX_TYPE_UINT32);
    break;
    
    case EUsage::STAGING:
      assert(false); // we shouldn't call bind on a staging buffer
    break;
  }
}
