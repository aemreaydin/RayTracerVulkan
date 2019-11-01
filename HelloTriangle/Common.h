#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <array>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
	static std::array<VkVertexInputAttributeDescription, 2> GetInputAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> inputAttributeDescriptions = {};

		// Position
		inputAttributeDescriptions[0].binding = 0;
		inputAttributeDescriptions[0].location = 0;
		inputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		inputAttributeDescriptions[0].offset = offsetof(SVertex, Position);

		inputAttributeDescriptions[1].binding = 0;
		inputAttributeDescriptions[1].location = 1;
		inputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		inputAttributeDescriptions[1].offset = offsetof(SVertex, Color);

		return inputAttributeDescriptions;
	}
	glm::vec4 Position;
	glm::vec4 Color;
};

struct SUniformBufferObject
{
	alignas(16) glm::mat4 Model;
	alignas(16) glm::mat4 View;
	alignas(16) glm::mat4 Projection;
};