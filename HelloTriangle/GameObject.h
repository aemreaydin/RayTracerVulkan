#pragma once
#include "CommonStructs.h"

#include <glm/glm.hpp>
#include <memory>

class IGameObject
{
public:
	explicit IGameObject(SObjectInformation, STransform);

	virtual ~IGameObject() = default;	
	IGameObject(const IGameObject&) = delete;
	IGameObject(IGameObject&&) = delete;
	IGameObject& operator=(const IGameObject&) = delete;
	IGameObject& operator=(IGameObject&&) = delete;

	virtual void Update() = 0;
	virtual void Draw() = 0;

	[[nodiscard]] VkBuffer& GetVertexBuffer();
	[[nodiscard]] VkBuffer& GetIndexBuffer();
	[[nodiscard]] VkDeviceMemory& GetVertexBufferMemory();
	[[nodiscard]] VkDeviceMemory& GetIndexBufferMemory();
	[[nodiscard]] const SVertex* GetVertexData() const;
	[[nodiscard]] const uint32_t* GetIndexData() const;
	[[nodiscard]] size_t GetVertexBufferSize() const;
	[[nodiscard]] size_t GetIndexBufferSize() const;
	[[nodiscard]] uint32_t GetIndexArraySize() const;

	void Cleanup(const VkDevice& device) const
	{
		vkDestroyBuffer(device, mModelInformation.VertexBuffer.Buffer, nullptr);
		vkDestroyBuffer(device, mModelInformation.IndexBuffer.Buffer, nullptr);
		vkDestroyBuffer(device, mModelInformation.UniformBuffer.Buffer, nullptr);
		vkFreeMemory(device, mModelInformation.VertexBuffer.BufferMemory, nullptr);
		vkFreeMemory(device, mModelInformation.IndexBuffer.BufferMemory, nullptr);
		vkFreeMemory(device, mModelInformation.UniformBuffer.BufferMemory, nullptr);
	}

	STransform mTransform{};
	SModelInformation mModelInformation;
	SObjectInformation mObjectInformation;
};

class CStaticGameObject final : public IGameObject
{
public:
	explicit CStaticGameObject(SObjectInformation, STransform);

	void Update() override;
	void Draw() override;
};

