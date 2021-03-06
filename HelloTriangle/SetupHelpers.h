#pragma once
#include "Common.h"

struct SQueueFamilyIndices
{
	// Contains no value until one is assigned. Can check if a value is assigned
	// using hasValue()
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;
	std::optional<uint32_t> TransferFamily;

	[[nodiscard]] bool IsComplete() const
	{
		return GraphicsFamily.has_value() && 
			PresentFamily.has_value() &&
			TransferFamily.has_value();
	}
};

struct SSwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR> PresentModes;
};

class CSetupHelpers
{
public:
	static std::vector<const char*> GetRequiredExtensions();
	static bool CheckExtensionSupport(const char** extensionsRequired,
	                                  uint32_t extensionsRequiredCount,
	                                  std::vector<VkExtensionProperties> supportedExtensions);
	static bool CheckDeviceExtensionsSupport(const VkPhysicalDevice& physicalDevice);
	static bool CheckValidationSupport();
	static bool IsDeviceSuitable(const VkPhysicalDevice& physicalDevice,
	                             const VkSurfaceKHR& surface);
	static SQueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device,
	                                             const VkSurfaceKHR& surface);
	static SSwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice,
	                                                      const VkSurfaceKHR& surface);
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D ChooseSwapExtent(GLFWwindow* pWindow, const VkSurfaceCapabilitiesKHR& capabilities);

	static uint32_t FindMemoryType(const VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static VkFormat FindSupportedFormat(const VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& vecFormat, const VkImageTiling& tiling, const VkFormatFeatureFlags& features);
	static VkFormat FindDepthFormat(const VkPhysicalDevice& physicalDevice);
	static bool HasStencilComponent(const VkFormat& format);
};
