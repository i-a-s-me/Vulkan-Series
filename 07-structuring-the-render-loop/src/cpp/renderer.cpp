// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <renderer.hpp>

#include <fstream>

static Result<std::vector<uint32_t>> load_shader_from_file(const std::string &path)
{
  std::ifstream f(path, std::ios::in | std::ios::binary | std::ios::ate);
  if (!f.is_open())
    return "Failed to open shader file \"" + path + "\".";

  std::vector<uint32_t> data;
  data.resize(f.tellg() >> 2);
  f.seekg(SEEK_SET);
  f.read((char *) data.data(), data.size() << 2);

  return std::move(data);
}

Result<Box<Renderer>> Renderer::create(
    std::function<Result<>(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback)
{
  Box<Renderer> renderer(new Renderer());

  VkExtent2D inital_extent{};

  CHECK_ASSIGN_RESULT(VulkanCore::create(surface_creation_callback, inital_extent), renderer->m_core);
  CHECK_ASSIGN_RESULT(Swapchain::create(renderer->m_core.get(), inital_extent), renderer->m_swapchain);
  renderer->on_swapchain_resize();
  renderer->m_swapchain->register_resize_callback(
      [renderer = renderer.get()](VkExtent2D) { renderer->on_swapchain_resize(); });

  CHECK_ASSIGN_RESULT(ShaderDataTable::create(renderer->m_core->get_device(),
                                              std::initializer_list<ShaderDataTable::EntryDescription>{
                                                  {
                                                      .name = "vertex_uniform_data",
                                                      .set = 0,
                                                      .binding = 0,
                                                      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      .count = 1,
                                                      .stages = VK_SHADER_STAGE_VERTEX_BIT,
                                                  },
                                                  {
                                                      .name = "texture_sampler",
                                                      .set = 1,
                                                      .binding = 0,
                                                      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                      .count = 1,
                                                      .stages = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                  },
                                              }),
                      renderer->m_shader_data_table);

  std::vector<uint32_t> vertex_shader_data, fragment_shader_data{};
  CHECK_ASSIGN_RESULT(load_shader_from_file("resources/default.vert.spv"), vertex_shader_data);
  CHECK_ASSIGN_RESULT(load_shader_from_file("resources/default.frag.spv"), fragment_shader_data);

  CHECK_ASSIGN_RESULT(
      Pipeline::create(
          renderer->m_core->get_device(), renderer->m_swapchain->get_color_format(),
          renderer->m_swapchain->get_depth_format(),
          std::initializer_list<VkVertexInputBindingDescription>{
              {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
          },
          std::initializer_list<VkVertexInputAttributeDescription>{
              VkVertexInputAttributeDescription{
                  .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
              VkVertexInputAttributeDescription{.location = 1,
                                                .binding = 0,
                                                .format = VK_FORMAT_R32G32B32_SFLOAT,
                                                .offset = offsetof(Vertex, colour)},
              VkVertexInputAttributeDescription{
                  .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, texcoord)},
          },
          renderer->m_shader_data_table.get(), vertex_shader_data, fragment_shader_data),
      renderer->m_graphics_pipeline);

  CHECK_ASSIGN_RESULT(
      Buffer::create(renderer->m_core->get_allocator(),
                     std::max(sizeof(CUBE_MESH_VERTICES), sizeof(CUBE_MESH_INDICES)), // maximum size we'll need for our staging operations
                     Buffer::EUsage::STAGING),
      renderer->m_staging_buffer);

  CHECK_ASSIGN_RESULT(Buffer::create(renderer->m_core->get_allocator(), sizeof(CUBE_MESH_VERTICES), Buffer::EUsage::VERTEX),
                      renderer->m_vertex_buffer);

  CHECK_ASSIGN_RESULT(Buffer::create(renderer->m_core->get_allocator(), sizeof(CUBE_MESH_INDICES), Buffer::EUsage::INDEX),
                      renderer->m_index_buffer);

  CHECK_ASSIGN_RESULT(
      Buffer::create(renderer->m_core->get_allocator(), sizeof(VertexUniformData), Buffer::EUsage::UNIFORM),
      renderer->m_vertex_uniform_buffer);

  {
    renderer->m_vertex_uniform_data.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    renderer->m_vertex_uniform_data.view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.75f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    if (!renderer->m_vertex_uniform_buffer->copy_from_cpu(&renderer->m_vertex_uniform_data, sizeof(m_vertex_uniform_data)))
      return "Failed to copy data to uniform buffer";
  }

  { // copy VERTICES to vertex buffer
    if (!renderer->m_staging_buffer->copy_from_cpu(CUBE_MESH_VERTICES, sizeof(CUBE_MESH_VERTICES)))
      return "Failed to copy VERTICES to staging buffer";

    CHECK_RESULT(renderer->m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
      VkBufferCopy copy_region{
          .srcOffset = 0,
          .dstOffset = 0,
          .size = renderer->m_vertex_buffer->get_size(),
      };

      vkCmdCopyBuffer(cmd, renderer->m_staging_buffer->get_handle(), renderer->m_vertex_buffer->get_handle(), 1,
                      &copy_region);

      VkMemoryBarrier barrier{
          .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
      };
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &barrier, 0,
                           nullptr, 0, nullptr);

      return Ok{};
    }));
  }

  { // copy INDICES to index buffer
    if (!renderer->m_staging_buffer->copy_from_cpu(CUBE_MESH_INDICES, sizeof(CUBE_MESH_INDICES)))
      return "Failed to copy INDICES to staging buffer";

    CHECK_RESULT(renderer->m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
      VkBufferCopy copy_region{
          .srcOffset = 0,
          .dstOffset = 0,
          .size = renderer->m_index_buffer->get_size(),
      };

      vkCmdCopyBuffer(cmd, renderer->m_staging_buffer->get_handle(), renderer->m_index_buffer->get_handle(), 1,
                      &copy_region);

      VkMemoryBarrier barrier{
          .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_INDEX_READ_BIT,
      };
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &barrier, 0,
                           nullptr, 0, nullptr);

      return Ok{};
    }));
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

    if (vkCreateSampler(renderer->m_core->get_device(), &create_info, nullptr, &renderer->m_texture_sampler) !=
        VK_SUCCESS)
      return "Failed to create the texture sampler";
  }

  {
    std::vector<uint8_t> pixels{};
    VkExtent2D extent{};

    if (!Image::load_rgba_from_file("resources/Ground037_1K-PNG_Color.png", pixels, extent))
      return "Failed to load texture data from the disk";

    CHECK_ASSIGN_RESULT(
        Image::create(renderer->m_core->get_device(), renderer->m_core->get_allocator(), VK_FORMAT_R8G8B8A8_SRGB,
                      extent,
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT // We want to sample this image from fragment shader
                                                          // and copy data from staging buffer into this image.
                      ),
        renderer->m_texture_image);

    CHECK_RESULT(renderer->m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
      if (!renderer->m_texture_image->transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        return "Failed to transition texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";

      CHECK_RESULT(renderer->m_texture_image->copy_from_cpu(cmd, pixels.data(), pixels.size()));

      if (!renderer->m_texture_image->transition_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        return "Failed to transition texture image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";

      return Ok{};
    }));
  }

  if (!renderer->m_shader_data_table->write_buffer_entry("vertex_uniform_data",
                                                         renderer->m_vertex_uniform_buffer.get()))
    return "Failed to write uniform buffer to shader data table";

  if (!renderer->m_shader_data_table->write_image_entry("texture_sampler", renderer->m_texture_image.get(),
                                                        renderer->m_texture_sampler))
    return "Failed to write texture image to shader data table";

  renderer->m_shader_data_table->flush_writes();

  return renderer;
}

Renderer::~Renderer()
{
  if (!m_core || !m_core->get_device())
    return;

  m_core->wait_idle();

  m_texture_image.reset();

  vkDestroySampler(m_core->get_device(), m_texture_sampler, nullptr);

  m_graphics_pipeline.reset();
  m_shader_data_table.reset();

  m_vertex_buffer.reset();
  m_index_buffer.reset();
  m_staging_buffer.reset();
  m_vertex_uniform_buffer.reset();
}

Result<> Renderer::render()
{
  {
    static constexpr float SPEED = glm::radians(90.0f);
    static auto start_time = std::chrono::steady_clock::now();

    const auto current_time = std::chrono::steady_clock::now();
    const auto seconds = std::chrono::duration<float>(current_time - start_time).count();

    m_vertex_uniform_data.model =
        glm::rotate(glm::mat4(1.0f), std::fmod(seconds * SPEED, glm::two_pi<float>()), glm::vec3(1.0f, 1.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
    m_vertex_uniform_buffer->copy_from_cpu(&m_vertex_uniform_data, sizeof(m_vertex_uniform_data));
  }

  VkCommandBuffer cmd;
  CHECK_ASSIGN_RESULT(m_swapchain->begin_frame(), cmd);

  if (cmd == VK_NULL_HANDLE)
    return Ok();

  m_graphics_pipeline->bind(cmd);

  m_vertex_buffer->bind(cmd);
  m_index_buffer->bind(cmd);

  m_shader_data_table->bind(cmd);

  vkCmdDrawIndexed(cmd, sizeof(CUBE_MESH_INDICES) / sizeof(CUBE_MESH_INDICES[0]), 1, 0, 0, 0);

  CHECK_RESULT(m_swapchain->end_frame());

  return Ok();
}

void Renderer::resize(uint32_t fb_width, uint32_t fb_height)
{
  m_swapchain->resize(fb_width, fb_height);
}

void Renderer::on_swapchain_resize()
{
  m_vertex_uniform_data.projection =
      glm::perspective(glm::half_pi<float>(), m_swapchain->get_aspect_ratio(), 0.1f, 1.0f);
  m_vertex_uniform_data.projection[1][1] *= -1.0f;
}
