// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "common.hpp"

class Pipeline
{
public:
    static Result<Box<Pipeline>> create(VkDevice device, VkFormat color_attachment_format, 
        std::span<const VkVertexInputBindingDescription> input_bindings,
        std::span<const VkVertexInputAttributeDescription> input_attributes,
        std::span<const uint32_t> vertex_shader_source, std::span<const uint32_t> fragment_shader_source);

    Pipeline(const Pipeline&) = delete;
    ~Pipeline();

    void bind(VkCommandBuffer cmd);

protected:
    Pipeline(VkDevice device): m_device(device) {}

private:
    VkDevice m_device{};

    VkPipeline m_handle{};
    VkPipelineLayout m_layout{};
};