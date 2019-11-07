#include "BufferManager.h"
#include "CommandBufferManager.h"
#include "GameObject.h"
#include "SetupHelpers.h"

void CBufferManager::CreateVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkQueue& queue, const VkCommandPool& commandPool, GameObjectUPtr& gameObject)
{
	const auto bufferSize = gameObject->GetVertexBufferSize();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(device, physicalDevice, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer,
				 stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, gameObject->GetVertexData(), bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(device, physicalDevice, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				 gameObject->GetVertexBuffer(),
				 gameObject->GetVertexBufferMemory());

	copyBuffer(device, commandPool, queue, stagingBuffer, gameObject->GetVertexBuffer(), bufferSize);

	// Cleanup staging buffer and memory
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CBufferManager::CreateIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkQueue& queue, const VkCommandPool& commandPool, GameObjectUPtr& gameObject)
{
	const auto bufferSize = gameObject->GetIndexBufferSize();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(device, physicalDevice, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer,
				 stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, gameObject->GetIndexData(), bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(device, physicalDevice, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				 gameObject->GetIndexBuffer(),
				 gameObject->GetIndexBufferMemory());

	copyBuffer(device, commandPool, queue, stagingBuffer, gameObject->GetIndexBuffer(), bufferSize);

	// Cleanup staging buffer and memory
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CBufferManager::CreateUniformBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkQueue& queue, const VkCommandPool& commandPool, GameObjectUPtr& gameObject)
{
	//const auto uboSize = sizeof(SUniformBufferObject);

	//vecUniformBuffer.resize(vecSwapChainImages.size());
	//vecUniformBufferMemory.resize(vecSwapChainImages.size());

	//for (size_t i = 0; i < vecUniformBuffer.size(); ++i)
	//{
	//	createBuffer(uboSize,
	//				 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	//				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	//				 vecUniformBuffer[i], vecUniformBufferMemory[i]);
	//}
}

void CBufferManager::createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice,const VkDeviceSize size, const VkBufferUsageFlags usageFlags,
	const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create the buffer.");
	}

	// Get Memory Requirements
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = CSetupHelpers::FindMemoryType(
		physicalDevice, memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate memory for the buffer.");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void CBufferManager::copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, VkQueue& queue, const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size)
{
	auto commandBuffer = CCommandBufferManager::BeginCommandBuffer(device, commandPool);
	// Copy vertex buffer using command buffer
	VkBufferCopy bufferCopy;
	bufferCopy.size = size;
	bufferCopy.dstOffset = 0;
	bufferCopy.srcOffset = 0;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
	// End command buffer
	CCommandBufferManager::EndCommandBuffer(device, commandPool, queue, commandBuffer);
}
