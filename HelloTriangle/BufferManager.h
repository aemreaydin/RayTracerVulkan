#pragma once
#include "TypeAliases.h"

#include <vulkan/vulkan_core.h>
class CBufferManager
{
public:
	static void CreateVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkQueue& queue, const VkCommandPool& commandPool, GameObjectUPtr& gameObject);
	static void CreateIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkQueue& queue, const VkCommandPool& commandPool, GameObjectUPtr& gameObject);
	static void CreateUniformBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkQueue& queue, const VkCommandPool& commandPool, GameObjectUPtr& gameObject);

private:
	static void createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size, const VkBufferUsageFlags usageFlags,
							 const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, VkQueue& queue, const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size);
};

