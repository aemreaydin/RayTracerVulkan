#pragma once

#include "Common.h"

class CDebugHelpers
{
public:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT msgType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserData);

	static void SetupDebugMessenger(const VkInstance& instance,
	                                const VkAllocationCallbacks* pAllocator,
	                                VkDebugUtilsMessengerEXT* pDebugMessenger);

	static void PopulateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	static VkResult CreateDebugUtilsMessengerExt(
		const VkInstance& instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerExt(
		const VkInstance& instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
};
