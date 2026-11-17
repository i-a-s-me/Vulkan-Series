// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <core.hpp>
#include <swapchain.hpp>

Result<Box<Swapchain>> Swapchain::create(const VulkanCore *core)
{
  Box<Swapchain> swapchain(new Swapchain(core));

  return swapchain;
}

Swapchain::~Swapchain()
{
  if (!m_core || !m_core->get_device())
    return;

  
}
