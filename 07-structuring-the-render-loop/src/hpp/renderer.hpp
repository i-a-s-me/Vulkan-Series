// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include "image.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"

class VulkanCore;
class Swapchain;

class Renderer
{
  public:
  Renderer() = default;
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  ~Renderer();

  Result<> initialize(std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback);

  Result<> render();

  void resize(uint32_t fb_width, uint32_t fb_height);

  private:
  Box<VulkanCore> m_core{};
  Box<Swapchain> m_swapchain{};

  Box<Pipeline> m_graphics_pipeline{};

  Box<Buffer> m_staging_buffer{};
  Box<Buffer> m_vertex_buffer{};
  Box<Buffer> m_index_buffer{};
  Box<Buffer> m_vertex_uniform_buffer{};

  Box<Image> m_texture_image{};
  VkSampler m_texture_sampler{};

  Box<ShaderDataTable> m_shader_data_table{};
};
