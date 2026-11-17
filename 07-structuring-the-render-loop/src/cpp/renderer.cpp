// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <renderer.hpp>

#include <set>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <utility>
#include <fstream>
#include <algorithm>

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 colour;
  glm::vec2 texcoord;
};

struct VertexUniformData
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
} g_vertex_uniform_data;

static const Vertex VERTICES[24] = {
  {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
  {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1.0f, 0.0f)},
  {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, 1.0f)  , glm::vec2(1.0f, 1.0f)},
  {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

  {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
  {glm::vec3( 0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1.0f, 0.0f)},
  {glm::vec3( 0.5f,  0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)  , glm::vec2(1.0f, 1.0f)},
  {glm::vec3(-0.5f,  0.5f, 0.5f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

  {glm::vec3( -0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
  {glm::vec3( -0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1.0f, 0.0f)},
  {glm::vec3(-0.5f,  0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)  , glm::vec2(1.0f, 1.0f)},
  {glm::vec3(-0.5f, -0.5f, 0.5f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

  {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
  {glm::vec3(0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1.0f, 0.0f)},
  {glm::vec3(0.5f,  0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)  , glm::vec2(1.0f, 1.0f)},
  {glm::vec3(0.5f, -0.5f, 0.5f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

  {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
  {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1.0f, 0.0f)},
  {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f)  , glm::vec2(1.0f, 1.0f)},
  {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

  {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
  {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1.0f, 0.0f)},
  {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f)  , glm::vec2(1.0f, 1.0f)},
  {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
};

static const uint32_t INDICES[36] = {
  0, 1, 2,
  2, 3, 0,

  4, 5, 6,
  6, 7, 4,

  8, 9, 10,
  10, 11, 8,

  12, 13, 14,
  14, 15, 12,

  16, 17, 18,
  18, 19, 16,

  20, 21, 22,
  22, 23, 20,
};

// Create checkerboard pattern pixels (violet and white)
static std::vector<uint8_t> get_checkerboard_pattern_pixels(uint32_t width, uint32_t height, uint32_t checker_size = 4)
{
  std::vector<uint8_t> rgba_data(width * height * 4);

  for (uint32_t y = 0; y < height; ++y)
  {
    for (uint32_t x = 0; x < width; ++x)
    {
      const bool is_violet = ((x / checker_size) % 2) == ((y / checker_size) % 2);
      uint32_t index = (y * width + x) * 4;
      if (is_violet)
      {
        rgba_data[index + 0] = 255;
        rgba_data[index + 1] = 0;
        rgba_data[index + 2] = 255;
        rgba_data[index + 3] = 255;
      }
      else
      {
        rgba_data[index + 0] = 255;
        rgba_data[index + 1] = 255;
        rgba_data[index + 2] = 255;
        rgba_data[index + 3] = 255;
      }
    }
  }

  return rgba_data;
}

static Result<std::vector<uint32_t>> load_shader_from_file(const std::string& path)
{
  std::ifstream f(path, std::ios::in | std::ios::binary | std::ios::ate);
  if (!f.is_open())
    return "Failed to open shader file \""+ path + "\".";

  std::vector<uint32_t> data;
  data.resize(f.tellg() >> 2);
  f.seekg(SEEK_SET);
  f.read((char*)data.data(), data.size() << 2);

  return std::move(data);
}

Renderer::~Renderer()
{
  if (m_device)
  {
    vkDeviceWaitIdle(m_device);

    m_texture_image.reset();
    
    vkDestroySampler(m_device, m_texture_sampler, nullptr);

    vkDestroyFence(m_device, m_single_shot_command_fence, nullptr);

    vkDestroyCommandPool(m_device, m_single_shot_command_pool, nullptr);

    m_graphics_pipeline.reset();
    m_shader_data_table.reset();

    m_vertex_buffer.reset();
    m_index_buffer.reset();
    m_staging_buffer.reset();
    m_vertex_uniform_buffer.reset();

    m_depth_attachment.reset();

    for (uint32_t i = 0; i < NUM_CONCURRENT_FRAMES; i++)
    {
      vkDestroyFence(m_device, m_frames[i].fen_in_flight, nullptr);
      vkDestroySemaphore(m_device, m_frames[i].sm_image_available, nullptr);
    }

    for (size_t i = 0; i < m_render_done_semaphores.size(); i++)
      vkDestroySemaphore(m_device, m_render_done_semaphores[i], nullptr);
    m_render_done_semaphores.clear();

    vkDestroyCommandPool(m_device, m_frame_resource_command_pool, nullptr);

    for (const auto &view : m_swapchain_image_views)
      vkDestroyImageView(m_device, view, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    vmaDestroyAllocator(m_allocator);

    vkDestroyDevice(m_device, nullptr);
  }

  if (m_instance)
  {
    if (m_surface)
      vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    {
      const auto debug_messenger_destroy_func =
          (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
      if (debug_messenger_destroy_func && m_debug_messenger)
        debug_messenger_destroy_func(m_instance, m_debug_messenger, nullptr);
    }

    vkDestroyInstance(m_instance, nullptr);
  }
}

bool Renderer::initialize(
    std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback)
{



  if (!create_device_and_swapchain())
    return false;

  if (!create_frame_resources())
    return false;

  CHECK_RESULT(
    ShaderDataTable::create(
      m_device,
      std::initializer_list<ShaderDataTable::EntryDescription>{
        {.name = "vertex_uniform_data", .set = 0, .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT,},
        {.name = "texture_sampler", .set = 1, .binding = 0, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1, .stages = VK_SHADER_STAGE_FRAGMENT_BIT,},
      }
    ),
    m_shader_data_table
  );

  std::vector<uint32_t> vertex_shader_data, fragment_shader_data{};
  CHECK_RESULT(load_shader_from_file("resources/default.vert.spv"), vertex_shader_data);
  CHECK_RESULT(load_shader_from_file("resources/default.frag.spv"), fragment_shader_data);

  CHECK_RESULT(
    Pipeline::create(
      m_device,
      m_swapchain_format.format,
      m_depth_attachment->get_format(),
      std::initializer_list<VkVertexInputBindingDescription>{
        {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
      },
      std::initializer_list<VkVertexInputAttributeDescription>{
        VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
        VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, colour)},
        VkVertexInputAttributeDescription{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, texcoord)},
      },
      m_shader_data_table.get(),
      vertex_shader_data,
      fragment_shader_data
    ),
    m_graphics_pipeline
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      std::max(sizeof(VERTICES), sizeof(INDICES)), // maximum size we'll need for our staging operations 
      Buffer::EUsage::STAGING
    ),
    m_staging_buffer
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      sizeof(VERTICES), 
      Buffer::EUsage::VERTEX
    ),
    m_vertex_buffer
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      sizeof(INDICES), 
      Buffer::EUsage::INDEX
    ),
    m_index_buffer
  );

  CHECK_RESULT(
    Buffer::create(
      m_allocator,
      sizeof(VertexUniformData),
      Buffer::EUsage::UNIFORM
    ),
    m_vertex_uniform_buffer
  );

  {
    g_vertex_uniform_data.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    g_vertex_uniform_data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.75f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    if (!m_vertex_uniform_buffer->copy_from_cpu(&g_vertex_uniform_data, sizeof(g_vertex_uniform_data))) {
      printf("Failed to copy data to uniform buffer\n");
      return false;
    }
  }

  { // copy VERTICES to vertex buffer
    if(!m_staging_buffer->copy_from_cpu(VERTICES, sizeof(VERTICES))) {
      printf("Failed to copy VERTICES to staging buffer\n");
      return false;
    }

    const auto success = execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<>
    {
      VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = m_vertex_buffer->get_size(),
      };

      vkCmdCopyBuffer(cmd, m_staging_buffer->get_handle(), m_vertex_buffer->get_handle(), 1, &copy_region);

      VkMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
      };
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                          0, 1, &barrier, 0, nullptr, 0, nullptr);

      return Ok{};
    });
    if(!success) {
      printf("Failed to copy staging buffer to vertex buffer\n");
      return false;
    }
  }

  { // copy INDICES to index buffer
    if(!m_staging_buffer->copy_from_cpu(INDICES, sizeof(INDICES))) {
      printf("Failed to copy INDICES to staging buffer\n");
      return false;
    }

    const auto success = execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<>
    {
      VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = m_index_buffer->get_size(),
      };
      
      vkCmdCopyBuffer(cmd, m_staging_buffer->get_handle(), m_index_buffer->get_handle(), 1, &copy_region);

      VkMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_INDEX_READ_BIT,
      };
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                          0, 1, &barrier, 0, nullptr, 0, nullptr);

      return Ok{};
    });
    if (!success) {
      printf("Failed to copy staging buffer to index buffer\n");
      return false;
    }
  }

  {
    VkSamplerCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };

    if (vkCreateSampler(m_device, &create_info, nullptr, &m_texture_sampler) != VK_SUCCESS) {
      printf("Failed to create the texture sampler\n");
      return false;
    }
  }

  {
    std::vector<uint8_t> pixels{};
    VkExtent2D extent{};

    if (!Image::load_rgba_from_file("resources/Ground037_1K-PNG_Color.png", pixels, extent)) {
      printf("Failed to load texture data from the disk\n");
      return false;
    }

    CHECK_RESULT(
      Image::create(
        m_device, 
        m_allocator,
        VK_FORMAT_R8G8B8A8_SRGB,
        extent,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT // We want to sample this image from fragment shader and 
                                                                          // copy data from staging buffer into this image.
      ),
    m_texture_image);

    const auto success = execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<>
    {
      if (!m_texture_image->transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        return "Failed to transition texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";

      if (!m_texture_image->copy_from_cpu(cmd, pixels.data(), pixels.size()))
        return "Failed to copy pixels to texture image";

      if (!m_texture_image->transition_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        return "Failed to transition texture image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";

      return Ok{};
    });
    if (!success) {
      printf("Failed to copy pixels to the image\n");
      return false;
    }
  }

  if (!m_shader_data_table->write_buffer_entry("vertex_uniform_data", m_vertex_uniform_buffer.get())) {
    printf("Failed to write uniform buffer to shader data table\n");
    return false;
  }
  if (!m_shader_data_table->write_image_entry("texture_sampler", m_texture_image.get(), m_texture_sampler)) {
    printf("Failed to write texture image to shader data table\n");
    return false;
  }
  m_shader_data_table->flush_writes();

  return true;
}

bool Renderer::render()
{
  const auto &current_frame = m_frames[m_current_frame_index];

  vkWaitForFences(m_device, 1, &current_frame.fen_in_flight, VK_TRUE, UINT64_MAX);

  uint32_t image_index{};
  switch (vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, current_frame.sm_image_available, VK_NULL_HANDLE,
                                &image_index))
  {
  case VK_ERROR_OUT_OF_DATE_KHR:
    if (!recreate_swapchain())
      return false;
    return true;

  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    break;

  default:
    printf("Failed to acquire swapchain image\n");
    return false;
  }

  VkCommandBufferBeginInfo cmd_begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkResetCommandBuffer(current_frame.cmd_buffer, 0);
  if (vkBeginCommandBuffer(current_frame.cmd_buffer, &cmd_begin_info) != VK_SUCCESS)
  {
    printf("Failed to start recording command buffer\n");
    return false;
  }

  if (!Image::transition_image_layout(current_frame.cmd_buffer, m_swapchain_images[image_index], VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
  {
    printf("Failed to transition swapchain image\n");
    return false;
  }

  draw_frame(current_frame.cmd_buffer, m_swapchain_image_views[image_index]);

  if (!Image::transition_image_layout(current_frame.cmd_buffer, m_swapchain_images[image_index],
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
  {
    printf("Failed to transition swapchain image\n");
    return false;
  }

  if (vkEndCommandBuffer(current_frame.cmd_buffer) != VK_SUCCESS)
  {
    printf("Failed to finish recording command buffer\n");
    return false;
  }

  VkPipelineStageFlags submit_pipeline_wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &current_frame.sm_image_available,
      .pWaitDstStageMask = &submit_pipeline_wait_stage,
      .commandBufferCount = 1,
      .pCommandBuffers = &current_frame.cmd_buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_render_done_semaphores[image_index],
  };

  vkResetFences(m_device, 1, &current_frame.fen_in_flight);
  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, current_frame.fen_in_flight) != VK_SUCCESS)
  {
    printf("Failed to submit frame command buffer\n");
    return false;
  }

  VkPresentInfoKHR present_info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_render_done_semaphores[image_index],
      .swapchainCount = 1,
      .pSwapchains = &m_swapchain,
      .pImageIndices = &image_index,
  };

  switch (vkQueuePresentKHR(m_present_queue, &present_info))
  {
  case VK_ERROR_OUT_OF_DATE_KHR:
  case VK_SUBOPTIMAL_KHR:
    if (!recreate_swapchain())
      return false;
    break;

  case VK_SUCCESS:
    break;

  default:
    printf("Failed to present swapchain\n");
    return false;
  }

  if (m_swapchain_is_out_of_date && !recreate_swapchain())
    return false;

  m_current_frame_index = (m_current_frame_index + 1) % NUM_CONCURRENT_FRAMES;

  return true;
}

void Renderer::draw_frame(VkCommandBuffer cmd, VkImageView color_attachment)
{
  {
    static constexpr float SPEED = glm::radians(90.0f);
    static auto start_time = std::chrono::steady_clock::now();
    
    const auto current_time = std::chrono::steady_clock::now();
    const auto seconds = std::chrono::duration<float>(current_time - start_time).count();

    g_vertex_uniform_data.model = 
      glm::rotate(glm::mat4(1.0f), std::fmod(seconds * SPEED, glm::two_pi<float>()), glm::vec3(1.0f, 1.0f, 1.0f))
      * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
    m_vertex_uniform_buffer->copy_from_cpu(&g_vertex_uniform_data, sizeof(g_vertex_uniform_data));
  }

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = color_attachment,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue =
          {
              .color =
                  {
                      0.1f,
                      0.1f,
                      0.1f,
                      1.0f,
                  },
          },
  };

  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_depth_attachment->get_view(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue =
          {
              .depthStencil =
                  {
                      1.0f,
                      0,
                  },
          },
  };

  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea =
          {
              {},
              m_swapchain_extent,
          },
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info,
  };

  vkCmdBeginRendering(cmd, &rendering_info);

  VkViewport viewport = {
    .width = (float)m_swapchain_extent.width,
    .height = (float)m_swapchain_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{
    .extent = m_swapchain_extent,
  };
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  m_graphics_pipeline->bind(cmd);

  m_vertex_buffer->bind(cmd);
  m_index_buffer->bind(cmd);

  m_shader_data_table->bind(cmd);

  vkCmdDrawIndexed(cmd, sizeof(INDICES)/sizeof(INDICES[0]), 1, 0, 0, 0);

  vkCmdEndRendering(cmd);
}

bool Renderer::execute_single_shot_commands(std::function<Result<>(VkCommandBuffer)> commands)
{
  if (vkResetCommandBuffer(m_single_shot_command_buffer, 0) != VK_SUCCESS)
    return false;

  VkCommandBufferBeginInfo begin_info{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  if (vkBeginCommandBuffer(m_single_shot_command_buffer, &begin_info) != VK_SUCCESS)
    return false;

  const auto result = commands(m_single_shot_command_buffer);
  if (result.is_error()) {
    printf("%s\n", result.get_error().c_str());
    return false;
  }

  if (vkEndCommandBuffer(m_single_shot_command_buffer) != VK_SUCCESS)
    return false;

  VkSubmitInfo submit_info{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &m_single_shot_command_buffer,
  };
  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_single_shot_command_fence) != VK_SUCCESS)
    return false;

  if (vkWaitForFences(m_device, 1, &m_single_shot_command_fence, true, UINT64_MAX) != VK_SUCCESS)
    return false;

  if (vkResetFences(m_device, 1, &m_single_shot_command_fence) != VK_SUCCESS)
    return false;

  return true;
}

void Renderer::resize(uint32_t fb_width, uint32_t fb_height)
{
  m_pending_swapchain_extent = {fb_width, fb_height};
  m_swapchain_is_out_of_date = true;
}

bool Renderer::create_device_and_swapchain()
{
  

  

  const auto surface_formats =
      vkEnumCall<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, m_physical_device, m_surface);
  const auto surface_present_modes =
      vkEnumCall<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, m_physical_device, m_surface);
  assert(surface_formats.size());
  assert(surface_present_modes.size());
  m_swapchain_format = surface_formats[0];
  m_swapchain_present_mode = (std::find(surface_present_modes.begin(), surface_present_modes.end(),
                                        VK_PRESENT_MODE_MAILBOX_KHR) != surface_present_modes.end())
                                 ? VK_PRESENT_MODE_MAILBOX_KHR
                                 : VK_PRESENT_MODE_FIFO_KHR;

  return recreate_swapchain();
}

bool Renderer::create_frame_resources()
{
  VkCommandPoolCreateInfo commmand_pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = m_graphics_queue_family_index,
  };
  if (vkCreateCommandPool(m_device, &commmand_pool_create_info, nullptr, &m_frame_resource_command_pool) != VK_SUCCESS)
  {
    printf("Failed to create command pool\n");
    return false;
  }

  VkCommandBufferAllocateInfo command_buffer_alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_frame_resource_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  for (uint32_t i = 0; i < NUM_CONCURRENT_FRAMES; i++)
  {
    if (vkAllocateCommandBuffers(m_device, &command_buffer_alloc_info, &m_frames[i].cmd_buffer) != VK_SUCCESS)
    {
      printf("Failed to allocate frame command buffer\n");
      return false;
    }

    VkSemaphoreCreateInfo semaphore_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    if (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].sm_image_available) != VK_SUCCESS)
    {
      printf("Failed to create a semaphore\n");
      return false;
    }

    VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    if (vkCreateFence(m_device, &fence_create_info, nullptr, &m_frames[i].fen_in_flight) != VK_SUCCESS)
    {
      printf("Failed to create a fence\n");
      return false;
    }
  }

  return true;
}

bool Renderer::recreate_swapchain()
{
  vkDeviceWaitIdle(m_device);

  VkSurfaceCapabilitiesKHR surface_caps{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surface_caps) != VK_SUCCESS)
  {
    printf("Failed to query physical device surface caps\n");
    return false;
  }

  // A minimized window has a 0x0 surface, and a 0x0 swapchain is not allowed. So we
  // keep the current swapchain, and try again later (once the window is restored).
  if ((surface_caps.maxImageExtent.width == 0) || (surface_caps.maxImageExtent.height == 0))
  {
    m_swapchain_is_out_of_date = true;
    return true;
  }

  m_swapchain_image_count = std::min(surface_caps.minImageCount + 1,
                                     (surface_caps.maxImageCount == 0) ? UINT32_MAX : surface_caps.maxImageCount);

  m_swapchain_pre_transform = surface_caps.currentTransform;

  m_pending_swapchain_extent.width = std::clamp(m_pending_swapchain_extent.width, surface_caps.minImageExtent.width,
                                                surface_caps.maxImageExtent.width);
  m_pending_swapchain_extent.height = std::clamp(m_pending_swapchain_extent.height, surface_caps.minImageExtent.height,
                                                 surface_caps.maxImageExtent.height);
  m_swapchain_extent = m_pending_swapchain_extent;

  m_depth_attachment.reset();
  for (const auto &view : m_swapchain_image_views)
    vkDestroyImageView(m_device, view, nullptr);
  if (m_swapchain)
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
  m_swapchain_image_views.clear();
  m_swapchain = VK_NULL_HANDLE;

  VkSwapchainCreateInfoKHR swapchain_create_info{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = m_surface,
      .minImageCount = m_swapchain_image_count,
      .imageFormat = m_swapchain_format.format,
      .imageColorSpace = m_swapchain_format.colorSpace,
      .imageExtent = m_swapchain_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = m_swapchain_pre_transform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = m_swapchain_present_mode,
      .clipped = VK_TRUE,
  };

  uint32_t queue_family_indices[] = {m_graphics_queue_family_index, m_present_queue_family_index};
  if (m_graphics_queue_family_index != m_present_queue_family_index)
  {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
  }

  if (vkCreateSwapchainKHR(m_device, &swapchain_create_info, nullptr, &m_swapchain) != VK_SUCCESS)
  {
    printf("Failed to create swapchain\n");
    return false;
  }

  m_swapchain_images = vkEnumCall<VkImage>(vkGetSwapchainImagesKHR, m_device, m_swapchain);

  // Note that we use the *actual* number of images here, which can be
  // more than the minImageCount we asked for.
  for (auto semaphore : m_render_done_semaphores)
    vkDestroySemaphore(m_device, semaphore, nullptr);
  m_render_done_semaphores.assign(m_swapchain_images.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_create_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (auto &semaphore : m_render_done_semaphores)
  {
    if (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &semaphore) != VK_SUCCESS)
    {
      printf("Failed to create a semaphore\n");
      return false;
    }
  }

  m_swapchain_image_views.reserve(m_swapchain_images.size());
  for (const auto &image : m_swapchain_images)
  {
    VkImageViewCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_swapchain_format.format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkImageView view{};
    if (vkCreateImageView(m_device, &create_info, nullptr, &view) != VK_SUCCESS)
    {
      printf("Failed to create a swapchain image view\n");
      return false;
    }

    m_swapchain_image_views.push_back(view);
  }

  CHECK_RESULT(
    Image::create(
      m_device,
      m_allocator,
      VK_FORMAT_D32_SFLOAT,
      m_swapchain_extent,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    ),
    m_depth_attachment
  );
  if (!execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<>
    {
      if (!m_depth_attachment->transition_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL))
        return "Failed to transition depth image layout";
    
      return Ok();
    })
  )
    return false;

  g_vertex_uniform_data.projection = glm::perspective(glm::half_pi<float>(), (float)m_swapchain_extent.width/(float)m_swapchain_extent.height, 0.1f, 1.0f);
  g_vertex_uniform_data.projection[1][1] *= -1.0f;

  m_swapchain_is_out_of_date = false;

  return true;
}
