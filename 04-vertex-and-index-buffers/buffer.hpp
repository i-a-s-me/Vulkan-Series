// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "common.hpp"

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

    Buffer(const Buffer&) = delete;
    ~Buffer();

    void bind(VkCommandBuffer cmd);

    VkDeviceSize get_size() const { return m_size; }

    VkBuffer get_handle() const { return m_handle; }

    void* get_mapped_ptr() const { assert(m_usage == EUsage::STAGING); return m_mapped_ptr; }

private:
    Buffer(VmaAllocator allocator): m_allocator(allocator) {}

private:
    EUsage m_usage{};
    VmaAllocator m_allocator{};
    
    VkBuffer m_handle{};
    VkDeviceSize m_size{};
    VmaAllocation m_allocation{};
    void* m_mapped_ptr{};
};