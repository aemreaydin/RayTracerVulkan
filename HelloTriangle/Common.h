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

#include "CommonStructs.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


const uint32_t WIDTH = 1200;
const uint32_t HEIGHT = 900;

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct SUniformBufferObject
{
	alignas(16) glm::mat4 Model;
	alignas(16) glm::mat4 View;
	alignas(16) glm::mat4 Projection;
};