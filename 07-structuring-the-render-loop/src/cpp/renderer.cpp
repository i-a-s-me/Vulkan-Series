// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <renderer.hpp>

#include <fstream>

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
    {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
    {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

    {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
    {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
    {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

    {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
    {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
    {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

    {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
    {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
    {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

    {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
    {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
    {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

    {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
    {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
    {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
    {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
};

static const uint32_t INDICES[36] = {
    0,  1,  2,  2,  3,  0,

    4,  5,  6,  6,  7,  4,

    8,  9,  10, 10, 11, 8,

    12, 13, 14, 14, 15, 12,

    16, 17, 18, 18, 19, 16,

    20, 21, 22, 22, 23, 20,
};

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

Result<> Renderer::initialize(
    std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback)
{
  VkExtent2D inital_extent{};

  CHECK_RESULT(VulkanCore::create(surface_creation_callback, inital_extent), m_core);

  CHECK_RESULT(Swapchain::create(m_core.get(), inital_extent), m_swapchain);

  on_swapchain_resize(inital_extent);

  m_swapchain->register_resize_callback([this](VkExtent2D extent) { this->on_swapchain_resize(extent); });

  CHECK_RESULT(ShaderDataTable::create(m_core->get_device(),
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
               m_shader_data_table);

  std::vector<uint32_t> vertex_shader_data, fragment_shader_data{};
  CHECK_RESULT(load_shader_from_file("resources/default.vert.spv"), vertex_shader_data);
  CHECK_RESULT(load_shader_from_file("resources/default.frag.spv"), fragment_shader_data);

  CHECK_RESULT(
      Pipeline::create(
          m_core->get_device(), m_swapchain->get_color_format(), m_swapchain->get_depth_format(),
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
          m_shader_data_table.get(), vertex_shader_data, fragment_shader_data),
      m_graphics_pipeline);

  CHECK_RESULT(
      Buffer::create(m_core->get_allocator(),
                     std::max(sizeof(VERTICES), sizeof(INDICES)), // maximum size we'll need for our staging operations
                     Buffer::EUsage::STAGING),
      m_staging_buffer);

  CHECK_RESULT(Buffer::create(m_core->get_allocator(), sizeof(VERTICES), Buffer::EUsage::VERTEX), m_vertex_buffer);

  CHECK_RESULT(Buffer::create(m_core->get_allocator(), sizeof(INDICES), Buffer::EUsage::INDEX), m_index_buffer);

  CHECK_RESULT(Buffer::create(m_core->get_allocator(), sizeof(VertexUniformData), Buffer::EUsage::UNIFORM),
               m_vertex_uniform_buffer);

  {
    g_vertex_uniform_data.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    g_vertex_uniform_data.view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.75f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    if (!m_vertex_uniform_buffer->copy_from_cpu(&g_vertex_uniform_data, sizeof(g_vertex_uniform_data)))
      return "Failed to copy data to uniform buffer";
  }

  { // copy VERTICES to vertex buffer
    if (!m_staging_buffer->copy_from_cpu(VERTICES, sizeof(VERTICES)))
      return "Failed to copy VERTICES to staging buffer";

    PROPAGATE_RESULT_ERROR(m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
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
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &barrier, 0,
                           nullptr, 0, nullptr);

      return Ok{};
    }));
  }

  { // copy INDICES to index buffer
    if (!m_staging_buffer->copy_from_cpu(INDICES, sizeof(INDICES)))
      return "Failed to copy INDICES to staging buffer";

    PROPAGATE_RESULT_ERROR(m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
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

    if (vkCreateSampler(m_core->get_device(), &create_info, nullptr, &m_texture_sampler) != VK_SUCCESS)
      return "Failed to create the texture sampler";
  }

  {
    std::vector<uint8_t> pixels{};
    VkExtent2D extent{};

    if (!Image::load_rgba_from_file("resources/Ground037_1K-PNG_Color.png", pixels, extent))
      return "Failed to load texture data from the disk";

    CHECK_RESULT(Image::create(m_core->get_device(), m_core->get_allocator(), VK_FORMAT_R8G8B8A8_SRGB, extent,
                               VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_TRANSFER_DST_BIT // We want to sample this image from fragment shader
                                                                   // and copy data from staging buffer into this image.
                               ),
                 m_texture_image);

    PROPAGATE_RESULT_ERROR(m_core->execute_single_shot_commands([&](VkCommandBuffer cmd) -> Result<> {
      if (!m_texture_image->transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        return "Failed to transition texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";

      PROPAGATE_RESULT_ERROR(m_texture_image->copy_from_cpu(cmd, pixels.data(), pixels.size()));

      if (!m_texture_image->transition_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        return "Failed to transition texture image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";

      return Ok{};
    }));
  }

  if (!m_shader_data_table->write_buffer_entry("vertex_uniform_data", m_vertex_uniform_buffer.get()))
    return "Failed to write uniform buffer to shader data table";

  if (!m_shader_data_table->write_image_entry("texture_sampler", m_texture_image.get(), m_texture_sampler))
    return "Failed to write texture image to shader data table";

  m_shader_data_table->flush_writes();

  return Ok();
}

Result<> Renderer::render()
{
  {
    static constexpr float SPEED = glm::radians(90.0f);
    static auto start_time = std::chrono::steady_clock::now();

    const auto current_time = std::chrono::steady_clock::now();
    const auto seconds = std::chrono::duration<float>(current_time - start_time).count();

    g_vertex_uniform_data.model =
        glm::rotate(glm::mat4(1.0f), std::fmod(seconds * SPEED, glm::two_pi<float>()), glm::vec3(1.0f, 1.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
    m_vertex_uniform_buffer->copy_from_cpu(&g_vertex_uniform_data, sizeof(g_vertex_uniform_data));
  }

  VkCommandBuffer cmd;
  CHECK_RESULT(m_swapchain->begin_frame(), cmd);

  if (cmd == VK_NULL_HANDLE)
    return Ok();

  m_graphics_pipeline->bind(cmd);

  m_vertex_buffer->bind(cmd);
  m_index_buffer->bind(cmd);

  m_shader_data_table->bind(cmd);

  vkCmdDrawIndexed(cmd, sizeof(INDICES) / sizeof(INDICES[0]), 1, 0, 0, 0);

  PROPAGATE_RESULT_ERROR(m_swapchain->end_frame());

  return Ok();
}

void Renderer::resize(uint32_t fb_width, uint32_t fb_height)
{
  m_swapchain->resize(fb_width, fb_height);
}

void Renderer::on_swapchain_resize(VkExtent2D extent)
{
  g_vertex_uniform_data.projection =
      glm::perspective(glm::half_pi<float>(), (float) extent.width / (float) extent.height, 0.1f, 1.0f);
  g_vertex_uniform_data.projection[1][1] *= -1.0f;
}
