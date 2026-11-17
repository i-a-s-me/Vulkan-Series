// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <shader_data_table.hpp>

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

      descriptor_counts[entry.type] += entry.count;

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
        return "Failed to create the descriptor set layout";
    }

    VkPipelineLayoutCreateInfo layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = (uint32_t)table->m_set_layouts.size(),
      .pSetLayouts = table->m_set_layouts.data(),
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
        .descriptorSetCount = (uint32_t)table->m_set_layouts.size(),
        .pSetLayouts = table->m_set_layouts.data(),
    };

    if (vkAllocateDescriptorSets(device, &alloc_info, table->m_sets.data()) != VK_SUCCESS)
      return "Failed to allocate descriptor sets";

    return table;
}

ShaderDataTable::~ShaderDataTable()
{
    if (!m_device)
        return;

    for (size_t i = 0; i < m_set_layouts.size(); i++)
      vkDestroyDescriptorSetLayout(m_device, m_set_layouts[i], nullptr);
    m_set_layouts.clear();

    if (m_pool) vkDestroyDescriptorPool(m_device, m_pool, nullptr);

    if (m_pipeline_layout) vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
}

bool ShaderDataTable::write_image_entry(const std::string& name, const Image* image, VkSampler sampler, uint32_t index)
{
    const auto entry = m_entry_map.find(name);
    if (entry == m_entry_map.end())
        return false;

    m_queued_write_image_infos.push_back(VkDescriptorImageInfo{
      sampler,
      image->get_view(),
      image->get_layout(),  
    });

    m_queued_writes.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_sets[entry->second.first],
        .dstBinding = entry->second.second,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &m_queued_write_image_infos.back(),
    });

    return true;
}

bool ShaderDataTable::write_buffer_entry(const std::string& name, const Buffer* buffer, VkDeviceSize offset, VkDeviceSize size, uint32_t index)
{
    const auto entry = m_entry_map.find(name);
    if (entry == m_entry_map.end())
        return false;

    m_queued_write_buffer_infos.push_back(VkDescriptorBufferInfo{
        buffer->get_handle(),
        offset,
        (size == 0) ? buffer->get_size() : size,
    });

    m_queued_writes.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_sets[entry->second.first],
        .dstBinding = entry->second.second,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // for now we assume all writes are uniform buffers
        .pBufferInfo = &m_queued_write_buffer_infos.back(),
    });

    return true;
}

void ShaderDataTable::flush_writes()
{
    if (m_queued_writes.empty())
        return;

    vkUpdateDescriptorSets(m_device, m_queued_writes.size(), m_queued_writes.data(), 0, nullptr);

    m_queued_write_image_infos.clear();
    m_queued_write_buffer_infos.clear();

    m_queued_writes.clear();
}

void ShaderDataTable::bind(VkCommandBuffer cmd)
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, m_sets.size(), m_sets.data(), 0, nullptr);
}
