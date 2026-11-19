// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include <pch.hpp>

class Buffer
{
  public:
  enum class EUsage
  {
    VERTEX,
    INDEX,
    STAGING,
  };

  public:
  static Result<Box<Buffer>> create(VmaAllocator allocator, VkDeviceSize size, EUsage usage);

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  ~Buffer();

  void bind(VkCommandBuffer cmd);

  VkDeviceSize get_size() const
  {
    return m_size;
  }

  VkBuffer get_handle() const
  {
    return m_handle;
  }

  bool copy_from_cpu(const void *data, VkDeviceSize size);

  private:
  Buffer(VmaAllocator allocator) : m_allocator(allocator)
  {
  }

  private:
  EUsage m_usage{};
  VmaAllocator m_allocator{};

  VkBuffer m_handle{};
  VkDeviceSize m_size{};
  VmaAllocation m_allocation{};
};

class IUniformBuffer
{
public:
  virtual ~IUniformBuffer() = default;
  
  virtual VkDeviceSize get_size() const = 0;
  virtual uint32_t get_frame_count() const = 0;
  virtual VkBuffer get_handle(uint32_t frame_index) const = 0;
};

template<typename DataT> class UniformBuffer: public IUniformBuffer
{
  public:
  static Result<Box<UniformBuffer>> create(VmaAllocator allocator, uint32_t frame_count);

  UniformBuffer(const UniformBuffer &) = delete;
  UniformBuffer &operator=(const UniformBuffer &) = delete;

  ~UniformBuffer();

  DataT &get_data()
  {
    return m_data;
  }

  const DataT &get_data() const
  {
    return m_data;
  }

  VkDeviceSize get_size() const override
  {
    return m_size;
  }

  VkBuffer get_handle(uint32_t frame_index) const override
  {
    return m_handles[frame_index];
  }

  uint32_t get_frame_count() const override
  {
    assert(m_handles.size() == m_allocations.size());
    return (uint32_t) m_handles.size();
  }

  bool flush(uint32_t frame_index);

  private:
  UniformBuffer(VmaAllocator allocator) : m_allocator(allocator)
  {
  }

  private:
  DataT m_data{};
  VmaAllocator m_allocator{};

  VkDeviceSize m_size{};
  std::vector<VkBuffer> m_handles{};
  std::vector<VmaAllocation> m_allocations{};
};

template<typename DataT>
Result<Box<UniformBuffer<DataT>>> UniformBuffer<DataT>::create(VmaAllocator allocator, uint32_t frame_count)
{
  Box<UniformBuffer> buffer{new UniformBuffer(allocator)};

  buffer->m_size = sizeof(DataT);

  VkBufferCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer->m_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // we only need to use our buffers from a single queue family at a time
  };

  VmaAllocationCreateInfo alloc_create_info{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
  };

  buffer->m_handles.resize(frame_count);
  buffer->m_allocations.resize(frame_count);
  for (uint32_t i = 0; i < frame_count; i++)
  {
    if (vmaCreateBuffer(allocator, &create_info, &alloc_create_info, &buffer->m_handles[i], &buffer->m_allocations[i],
                        nullptr) != VK_SUCCESS)
    {
      return "Failed to create the buffer";
    }
  }

  return buffer;
}

template<typename DataT> UniformBuffer<DataT>::~UniformBuffer()
{
  if (!m_allocator)
    return;

  const auto frame_count = get_frame_count(); // using the member function here so it triggers the assert as well
  for (uint32_t i = 0; i < frame_count; i++)
  {
    vmaDestroyBuffer(m_allocator, m_handles[i], m_allocations[i]);
  }
  m_handles.clear();
  m_allocations.clear();
}

template<typename DataT> bool UniformBuffer<DataT>::flush(uint32_t frame_index)
{
  return vmaCopyMemoryToAllocation(m_allocator, &m_data, m_allocations[frame_index], 0, sizeof(m_data)) == VK_SUCCESS;
}
