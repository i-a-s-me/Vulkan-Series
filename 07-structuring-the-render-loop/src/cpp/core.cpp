// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <core.hpp>

Result<Box<VulkanCore>> VulkanCore::create(std::function<bool(VkInstance, uint32_t &, uint32_t &, VkSurfaceKHR &)> surface_creation_callback)
{
  Box<VulkanCore> core(new VulkanCore());

  PROPAGATE_RESULT_ERROR(core->create_instance());
  PROPAGATE_RESULT_ERROR(core->select_physical_device());
  PROPAGATE_RESULT_ERROR(core->create_device());

  return core;
}

Result<> VulkanCore::create_instance()
{
  return Ok();
}

Result<> VulkanCore::select_physical_device()
{
  return Ok();
}

Result<> VulkanCore::create_device()
{
  return Ok();
}
