#include "SetupHelpers.h"

std::vector<const char*> CSetupHelpers::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions =
		glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> vecExtensions(glfwExtensions,
										   glfwExtensions + glfwExtensionCount);

#ifdef _DEBUG
	vecExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	return vecExtensions;
}

bool CSetupHelpers::checkExtensionSupport(const char** extensionsRequired, const uint32_t extensionsRequiredCount, std::vector<VkExtensionProperties> supportedExtensions)
{
	for (int i = 0; i != extensionsRequiredCount; i++)
	{
		const char* extension = extensionsRequired[i];
		auto iter = std::find_if(
			supportedExtensions.begin(), supportedExtensions.end(),
			[&extension](const VkExtensionProperties& extensionProperty)
			{
				return strcmp(extension, extensionProperty.extensionName) == 0;
			});
		if (iter == supportedExtensions.end()) return false;
	}
	return true;
}

bool CSetupHelpers::checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t deviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
										 &deviceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> vecExtensions(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(
		physicalDevice, nullptr, &deviceExtensionCount, vecExtensions.data());

	// If found an extension, remove it from the set, if the set empty you have
	// everything you need
	std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(),
											 DEVICE_EXTENSIONS.end());
	for (const auto& extension : vecExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}
bool CSetupHelpers::checkValidationSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto& layer : VALIDATION_LAYERS)
	{
		auto iter =
			std::find_if(availableLayers.begin(), availableLayers.end(),
						 [&layer](const VkLayerProperties& layerProperty)
						 {
							 return strcmp(layerProperty.layerName, layer) == 0;
						 });
		if (iter == availableLayers.end()) return false;
	}
	return true;
}
bool CSetupHelpers::isDeviceSuitable(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	// Find if all queues have a value
	SQueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	// Check just for swapchain support for now
	bool swapChainAdequate = false;
	bool extensionsSupported =
		CSetupHelpers::checkDeviceExtensionsSupport(physicalDevice);
	if (extensionsSupported)
	{
		SSwapChainSupportDetails details =
			querySwapChainSupport(physicalDevice, surface);
		swapChainAdequate =
			!details.surfaceFormats.empty() && !details.presentModes.empty();
	}
	return deviceProperties.deviceType ==
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		deviceFeatures.geometryShader && indices.isComplete() &&
		swapChainAdequate;
}

SQueueFamilyIndices CSetupHelpers::findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface)
{
	SQueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
											 nullptr);
	std::vector<VkQueueFamilyProperties> vecQueueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
											 vecQueueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : vecQueueFamilies)
	{
		if (queueFamily.queueCount > 0 &&
			queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}
	return indices;
}

SSwapChainSupportDetails CSetupHelpers::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SSwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
											  &details.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
										 nullptr);
	if (formatCount != 0)
	{
		details.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice, surface, &formatCount, details.surfaceFormats.data());
	}

	uint32_t presentCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
											  &presentCount, nullptr);
	if (presentCount != 0)
	{
		details.presentModes.resize(presentCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice, surface, &presentCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR CSetupHelpers::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	auto iter = std::find_if(availableFormats.begin(), availableFormats.end(),
							 [](const VkSurfaceFormatKHR& format)
							 {
								 return format.colorSpace ==
									 VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
									 format.format == VK_FORMAT_B8G8R8A8_UNORM;
							 });
	if (iter != availableFormats.end())
	{
		return *iter;
	}
	return availableFormats[0];
}

VkPresentModeKHR CSetupHelpers::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	auto iter =
		std::find_if(availablePresentModes.begin(), availablePresentModes.end(),
					 [](const VkPresentModeKHR& presentMode)
					 {
						 return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
					 });
	if (iter != availablePresentModes.end())
	{
		return *iter;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D CSetupHelpers::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width !=
		std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { WIDTH, HEIGHT };
		actualExtent.width =
			std::clamp(actualExtent.width, capabilities.minImageExtent.width,
					   capabilities.maxImageExtent.width);
		actualExtent.height =
			std::clamp(actualExtent.height, capabilities.minImageExtent.height,
					   capabilities.maxImageExtent.height);
		return actualExtent;
	}
}
