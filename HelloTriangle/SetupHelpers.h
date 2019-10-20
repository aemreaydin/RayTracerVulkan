#pragma once
#include "Common.h"

struct SQueueFamilyIndices
{
	// Contains no value until one is assigned. Can check if a value is assigned
	// using hasValue()
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SSwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};

class CSetupHelpers
{
public:
	static std::vector<const char*> getRequiredExtensions();
	static bool checkExtensionSupport(const char** extensionsRequired, 
									  const uint32_t extensionsRequiredCount, 
									  std::vector<VkExtensionProperties> supportedExtensions);
	static bool checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice);
	static bool checkValidationSupport();
	static bool isDeviceSuitable(const VkPhysicalDevice physicalDevice,
								 const VkSurfaceKHR surface);
	static SQueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device,
												 const VkSurfaceKHR surface);
	static SSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, 
														  VkSurfaceKHR surface);
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};
