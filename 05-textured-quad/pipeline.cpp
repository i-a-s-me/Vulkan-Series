// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "pipeline.hpp"

Result<Box<Pipeline>> Pipeline::create(VkDevice device, VkFormat color_attachment_format,
        std::span<const VkVertexInputBindingDescription> input_bindings,
        std::span<const VkVertexInputAttributeDescription> input_attributes,
        std::span<const uint32_t> vertex_shader_source, std::span<const uint32_t> fragment_shader_source)
{
  Box<Pipeline> pipeline{new Pipeline(device)};

  VkPipelineLayoutCreateInfo layout_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pushConstantRangeCount = 0,
  };

  if (vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline->m_layout) != VK_SUCCESS)
    return "Failed to create the graphics pipeline layout";

  VkShaderModule shader_modules[2];

  VkShaderModuleCreateInfo shader_module_create_infos[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = vertex_shader_source.size() << 2,
      .pCode = vertex_shader_source.data(),
    },
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = fragment_shader_source.size() << 2,
      .pCode = fragment_shader_source.data(),
    },
  };

  if (vkCreateShaderModule(device, &shader_module_create_infos[0], nullptr, &shader_modules[0]) != VK_SUCCESS)
    return "Failed to create the shader module for vertex shader";

  if (vkCreateShaderModule(device, &shader_module_create_infos[1], nullptr, &shader_modules[1]) != VK_SUCCESS) {
    vkDestroyShaderModule(device, shader_modules[0], nullptr);
    return "Failed to create the shader module for fragment shader";
  }

  VkPipelineShaderStageCreateInfo shader_stage_create_infos[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = shader_modules[0],
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = shader_modules[1],
      .pName = "main",
    },
  };

  VkPipelineRenderingCreateInfo rendering_create_info {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &color_attachment_format,
    .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = (uint32_t)input_bindings.size(),
    .pVertexBindingDescriptions = input_bindings.data(),
    .vertexAttributeDescriptionCount = (uint32_t)input_attributes.size(),
    .pVertexAttributeDescriptions = input_attributes.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo ia_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo rs_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .lineWidth = 1.0,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment_state{
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_state,
  };

  VkDynamicState dynamic_states[2] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = (sizeof(dynamic_states)/sizeof(dynamic_states[0])),
    .pDynamicStates = dynamic_states,
  };

  VkGraphicsPipelineCreateInfo create_info{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &rendering_create_info,

    .stageCount = (sizeof(shader_stage_create_infos)/sizeof(shader_stage_create_infos[0])),
    .pStages = shader_stage_create_infos,

    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &ia_state_create_info,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rs_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDynamicState = &dynamic_state_create_info,

    .layout = pipeline->m_layout,
  };

  if (vkCreateGraphicsPipelines(device, nullptr, 1, &create_info, nullptr, &pipeline->m_handle) != VK_SUCCESS) {
    vkDestroyShaderModule(device, shader_modules[0], nullptr);
    vkDestroyShaderModule(device, shader_modules[1], nullptr);

    return "Failed to create the graphics pipeline";
  }

  vkDestroyShaderModule(device, shader_modules[0], nullptr);
  vkDestroyShaderModule(device, shader_modules[1], nullptr);

  return pipeline;
}

void Pipeline::bind(VkCommandBuffer cmd)
{
  // for now we'll only be using graphics pipeline so we can just hardcode VK_PIPELINE_BIND_POINT_GRAPHICS here
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_handle);
}

Pipeline::~Pipeline()
{
  if (!m_device)
    return;

  if (m_handle) vkDestroyPipeline(m_device, m_handle, nullptr);
  if (m_layout) vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}
