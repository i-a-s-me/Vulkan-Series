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

    std::map<VkDescriptorType, uint32_t> descriptor_counts{};
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> bindings{set_count};
    for(const auto& entry: entries) {
      bindings[entry.set].push_back({
        .binding = entry.binding,
        .descriptorType = entry.type,
        .descriptorCount = entry.count,
        .stageFlags = entry.stages,
      });

      if (descriptor_counts.contains(entry.type))
          descriptor_counts[entry.type] += entry.count;
      else
          descriptor_counts[entry.type] = 1;

      table->m_entry_map[entry.name] = {entry.set, entry.binding};
    }

    std::vector<VkDescriptorSetLayout> set_layouts{set_count};
    for(uint32_t i = 0; i < set_count; i++) {
      VkDescriptorSetLayoutCreateInfo set_layout_create_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (uint32_t)bindings[i].size(),
        .pBindings = bindings[i].data(),
      };

      if (vkCreateDescriptorSetLayout(device, &set_layout_create_info, nullptr, &set_layouts[i]) != VK_SUCCESS)
        return "Failed to create the descriptor set layout";
    }

    VkPipelineLayoutCreateInfo layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = (uint32_t)set_layouts.size(),
      .pSetLayouts = set_layouts.data(),
      .pushConstantRangeCount = 0, // we'll talk about push constants in another post
    };

    if (vkCreatePipelineLayout(device, &layout_create_info, nullptr, &table->m_pipeline_layout) != VK_SUCCESS)
      return "Failed to create the pipeline layout";

    std::vector<VkDescriptorPoolSize> pool_sizes{};
    for(const auto& dc: descriptor_counts) {
      pool_sizes.push_back({.type = dc.first, .descriptorCount = dc.second});
    }

    VkDescriptorPoolCreateInfo pool_create_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = set_count,
      .poolSizeCount = (uint32_t)pool_sizes.size(),
      .pPoolSizes = pool_sizes.data(),
    };

    if (vkCreateDescriptorPool(device, &pool_create_info, nullptr, &table->m_pool) != VK_SUCCESS)
      return "Failed to create the descriptor pool";

    table->m_sets.resize(set_count);
    VkDescriptorSetAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = table->m_pool,
        .descriptorSetCount = (uint32_t)set_layouts.size(),
        .pSetLayouts = set_layouts.data(),
    };

    if (vkAllocateDescriptorSets(device, &alloc_info, table->m_sets.data()) != VK_SUCCESS)
      return "Failed to allocate descriptor sets";

    for (size_t i = 0; i < set_layouts.size(); i++)
      vkDestroyDescriptorSetLayout(device, set_layouts[i], nullptr);
    set_layouts.clear();

    return table;
}

ShaderDataTable::~ShaderDataTable()
{
    if (!m_device)
        return;

    if (m_pool) vkDestroyDescriptorPool(m_device, m_pool, nullptr);

    if (m_pipeline_layout) vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
}
