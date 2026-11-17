// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "image.hpp"

class Pipeline;

class ShaderDataTable
{
public:
    struct EntryDescription
    {
        std::string name{};
        uint32_t set{};
        uint32_t binding{};
        VkDescriptorType type{};
        uint32_t count{};
        VkShaderStageFlags stages{};
    };

public:
    static Result<Box<ShaderDataTable>> create(VkDevice device, std::span<const EntryDescription> entries);

    ShaderDataTable(const ShaderDataTable&) = delete;
    ~ShaderDataTable();

    bool write_image_entry(const std::string& name, const Image* image, VkSampler sampler, uint32_t index = 0);
    bool write_buffer_entry(const std::string& name, const Buffer* buffer, VkDeviceSize offset = 0, VkDeviceSize size = 0, uint32_t index = 0);
    void flush_writes();

    void bind(VkCommandBuffer cmd);

    VkPipelineLayout get_pipeline_layout() const { return m_pipeline_layout; }

protected:
    ShaderDataTable(VkDevice device): m_device(device) {}

private:
    VkDevice m_device{};

    VkDescriptorPool m_pool{};
    VkPipelineLayout m_pipeline_layout{};
    std::vector<VkDescriptorSet> m_sets{};
    std::vector<VkDescriptorSetLayout> m_set_layouts{};
    std::map<std::string, std::pair<uint32_t, uint32_t>> m_entry_map{};

    std::vector<VkWriteDescriptorSet> m_queued_writes{};
    std::deque<VkDescriptorImageInfo> m_queued_write_image_infos{};
    std::deque<VkDescriptorBufferInfo> m_queued_write_buffer_infos{};
};