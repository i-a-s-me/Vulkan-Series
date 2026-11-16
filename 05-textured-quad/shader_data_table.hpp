// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "common.hpp"

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

    VkPipelineLayout get_pipeline_layout() const { return m_pipeline_layout; }

protected:
    ShaderDataTable(VkDevice device): m_device(device) {}

private:
    VkDevice m_device{};

    VkDescriptorPool m_pool{};
    VkPipelineLayout m_pipeline_layout{};
    std::vector<VkDescriptorSet> m_sets{};
    std::map<std::string, std::pair<uint32_t, uint32_t>> m_entry_map{};
};