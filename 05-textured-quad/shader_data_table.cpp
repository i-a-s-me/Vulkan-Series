// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "shader_data_table.hpp"

Result<Box<ShaderDataTable>> ShaderDataTable::create(VkDevice device, std::span<const EntryDescription> entries)
{
  Box<ShaderDataTable> table{new ShaderDataTable(device)};

  uint32_t set_count{};
  for(const auto& entry: entries) {
    set_count = std::max(entry.set + 1, set_count);    
  }

  std::vector<std::vector<VkDescriptorSetLayoutBinding>> bindings{set_count};
  for(const auto& entry: entries) {
    bindings[entry.set].push_back({
      .binding = entry.binding,
      .descriptorType = entry.type,
      .descriptorCount = entry.count,
      .stageFlags = entry.stages,
    });
    table->m_entry_map[entry.name] = {entry.set, entry.binding}; 
  }

  table->m_set_layouts.resize(set_count);
  for(uint32_t i = 0; i < set_count; i++) {
    VkDescriptorSetLayoutCreateInfo set_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = (uint32_t)bindings[i].size(),
      .pBindings = bindings[i].data(),
    };

    if (vkCreateDescriptorSetLayout(device, &set_layout_create_info, nullptr, &table->m_set_layouts[i]) != VK_SUCCESS)
      return "Failed to create the graphics pipeline descriptor set layout";
  }

  VkPipelineLayoutCreateInfo layout_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = (uint32_t)table->m_set_layouts.size(),
    .pSetLayouts = table->m_set_layouts.data(),
    .pushConstantRangeCount = 0, // we'll talk about push constants in another post
  };

  if (vkCreatePipelineLayout(device, &layout_create_info, nullptr, &table->m_pipeline_layout) != VK_SUCCESS)
    return "Failed to create the graphics pipeline layout";

  return table;
}

ShaderDataTable::~ShaderDataTable()
{
    if (!m_device)
        return;

    for (size_t i = 0; i < m_set_layouts.size(); i++)
        vkDestroyDescriptorSetLayout(m_device, m_set_layouts[i], nullptr);
    m_set_layouts.clear();

    if (m_pipeline_layout) vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
}
