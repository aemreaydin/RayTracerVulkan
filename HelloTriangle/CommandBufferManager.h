#pragma once

#include <vulkan/vulkan_core.h>


class CCommandBufferManager
{
public:
	[[nodiscard]] static VkCommandBuffer BeginCommandBuffer(const VkDevice& device, const VkCommandPool& commandPool);
	static void EndCommandBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue, VkCommandBuffer& commandBuffer);
};

