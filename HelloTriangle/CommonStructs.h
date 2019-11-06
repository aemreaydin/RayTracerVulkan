#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <iostream>
#include <array>
#include <vector>

struct STransform
{
	glm::vec3 Position;
	glm::vec3 Rotation;
	glm::vec3 Scale;
};

struct SVertex
{
	static VkVertexInputBindingDescription GetInputBindingDescription()
	{
		VkVertexInputBindingDescription inputBindingDescription;
		inputBindingDescription.binding = 0;
		inputBindingDescription.stride = sizeof(SVertex);
		inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return inputBindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetInputAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> inputAttributeDescriptions = {};

		// Position
		inputAttributeDescriptions[0].binding = 0;
		inputAttributeDescriptions[0].location = 0;
		inputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		inputAttributeDescriptions[0].offset = offsetof(SVertex, Position);
		// Normal
		inputAttributeDescriptions[1].binding = 0;
		inputAttributeDescriptions[1].location = 1;
		inputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		inputAttributeDescriptions[1].offset = offsetof(SVertex, Normal);
		// Texture Coordinates
		inputAttributeDescriptions[2].binding = 0;
		inputAttributeDescriptions[2].location = 2;
		inputAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		inputAttributeDescriptions[2].offset = offsetof(SVertex, TextureCoords);

		return inputAttributeDescriptions;
	}

	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TextureCoords;
};

struct SModelInformation
{
	std::vector<SVertex> VecVertex;
	std::vector<uint32_t> VecIndex;
};

struct SObjectInformation
{
	explicit SObjectInformation()
		: Filename(""), Transform(STransform()){ }
	std::string Filename;
	STransform Transform;
	SModelInformation ModelInformation;
};