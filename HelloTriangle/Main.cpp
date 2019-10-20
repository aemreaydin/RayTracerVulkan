#include "Common.h"
#include "DebugHelpers.h"
#include <map>
#include <set>
#include <optional>
#include <limits>

struct QueueFamilyIndices {
	// Contains no value until one is assigned. Can check if a value is assigned using hasValue()
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};
class HelloTriangleApp {
public:
	void Run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	void initWindow() {
		glfwInit();
		// Don't create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		pWindow = glfwCreateWindow(WIDTH, HEIGHT, "RayTracer Vulkan", nullptr, nullptr);
	}
	void initVulkan() {
		createInstance();
		if (enableValidationLayers) {
			CDebugHelpers::SetupDebugMessenger(instance, nullptr, &debugMessenger);
		}
		// Create the surface before the physical device
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
	}
	void mainLoop() {
		while (!glfwWindowShouldClose(pWindow)) {
			glfwPollEvents();
		}
	}
	void cleanup() {
		
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers) {
			CDebugHelpers::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(pWindow);
		glfwTerminate();
	}
private:
	void createInstance() {
		// Enable validation layers in Debug
		if (enableValidationLayers && !checkValidationSupport()) {
			throw std::runtime_error("Validation layers requested, but not supported.");
		}

		// Create the application information before creating the instance
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_1;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Engine";

		auto vecExtensions = getRequiredExtensions();

		// First query for the number of extensions
		uint32_t optionalExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &optionalExtensionCount, nullptr);
		std::vector<VkExtensionProperties> vecExtProperties(optionalExtensionCount);
		// Now get the properties
		vkEnumerateInstanceExtensionProperties(nullptr, &optionalExtensionCount, vecExtProperties.data());
		std::cout << vecExtensions.size() << " extensions required by GFLW." << std::endl;
		for (const auto& extension : vecExtensions) {
			std::cout << extension << std::endl;
		}
		std::cout << optionalExtensionCount << " instance extensions supported by Vulkan." << std::endl;
		for (const auto& extension : vecExtProperties) {
			std::cout << extension.extensionName << std::endl;
		}

		// Check if all the required extensions are supported by Vulkan
		bool isSupported = checkExtensionSupport(vecExtensions.data(), static_cast<uint32_t>(vecExtensions.size()), vecExtProperties);
		isSupported ? std::cout << "All the extensions are supported" << std::endl
			: throw std::runtime_error("Some extensions are not supported.");

		// Create the instance information before creating the instance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(vecExtensions.size());
		createInfo.ppEnabledExtensionNames = vecExtensions.data();

		// Create DebugCreateInfo to check for any problems during the creation of the instance
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			CDebugHelpers::PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// Now that we have the VkInstanceCreateInfo, we can create the instance
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VkInstance.");
		}
	}
	void createSurface() {
		if (glfwCreateWindowSurface(instance, pWindow, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the surface.");
		}
	}
	void pickPhysicalDevice() {
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		if (physicalDeviceCount == 0) {
			throw std::runtime_error("Failed to find a GPU with Vulkan support.");
		}
		std::vector<VkPhysicalDevice> vecDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, vecDevices.data());
		for (const auto& device : vecDevices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}
		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU.");
		}
	}
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		float queuePriority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> vecQueueCreateInfos;
		// If the indices are the same, we'll end up using one VkDeviceQueueCreateInfo
		std::set<uint32_t> setQueueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		for (auto queueFamilyIndex : setQueueFamilyIndices) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			vecQueueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(vecQueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = vecQueueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		// VkDeviceCreateInfo validation layers are no longer used
		// Validation Layers created when creating the VkInstance
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers) {
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames= validationLayers.data();
		}
		else {
			deviceCreateInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device.");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}
	void createSwapChain() {
		SwapChainSupportDetails details = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(details.surfaceFormats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
		VkExtent2D extent = chooseSwapExtent(details.surfaceCapabilities);

		uint32_t imageCount = details.surfaceCapabilities.minImageCount + 1;
		if (details.surfaceCapabilities.maxImageCount > 0 && imageCount > details.surfaceCapabilities.maxImageCount) {
			imageCount = details.surfaceCapabilities.maxImageCount;
		}

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		createInfo.preTransform = details.surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the swapchain.");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = format.format;
		swapChainExtent = extent;
	}
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		int index = 0;
		for (const auto& image : swapChainImages) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[index++]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create the image view.");
			}
		}
	}
	bool isDeviceSuitable(VkPhysicalDevice physicalDevice) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		// Find if all queues have a value
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		// Check just for swapchain support for now
		bool swapChainAdequate = false;
		bool extensionsSupported = checkDeviceExtensionsSupport(physicalDevice);
		if (extensionsSupported) {
			SwapChainSupportDetails details = querySwapChainSupport(physicalDevice);
			swapChainAdequate = !details.surfaceFormats.empty() && !details.presentModes.empty();
		}
		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader &&
			indices.isComplete() &&
			swapChainAdequate;
	}
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> vecQueueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, vecQueueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : vecQueueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}
		return indices;
	}
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.surfaceCapabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.surfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.surfaceFormats.data());
		}

		uint32_t presentCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
		if (presentCount != 0) {
			details.presentModes.resize(presentCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, details.presentModes.data());
		}

		return details;
	}
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		auto iter = std::find_if(availableFormats.begin(), availableFormats.end(), [](const VkSurfaceFormatKHR& format) {
			return format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_UNORM;
			});
		if (iter != availableFormats.end()) {
			return *iter;
		}
		return availableFormats[0];;
	}
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		auto iter = std::find_if(availablePresentModes.begin(), availablePresentModes.end(), [](const VkPresentModeKHR& presentMode) {
			return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
			});
		if (iter != availablePresentModes.end())
		{
			return *iter;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = { WIDTH, HEIGHT };
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actualExtent;
		}
	}
	bool checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice) {
		uint32_t deviceExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> vecExtensions(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, vecExtensions.data());

		// If found an extension, remove it from the set, if the set empty you have everything you need
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : vecExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}		
		return requiredExtensions.empty();
	}
	bool checkExtensionSupport(const char** extensionsRequired, const uint32_t extensionsRequiredCount, std::vector<VkExtensionProperties> supportedExtensions) {
		for (int i = 0; i != extensionsRequiredCount; i++) {
			const char* extension = extensionsRequired[i];
			auto iter = std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&extension](const VkExtensionProperties& extensionProperty) {
				return strcmp(extension, extensionProperty.extensionName) == 0;
				});
			if (iter == supportedExtensions.end()) return false;
		}
		return true;
	}
	bool checkValidationSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const auto& layer : validationLayers) {
			auto iter = std::find_if(availableLayers.begin(), availableLayers.end(), [&layer](const VkLayerProperties& layerProperty) {
				return strcmp(layerProperty.layerName, layer) == 0;
				});
			if (iter == availableLayers.end()) return false;
		}
		return true;
	}
	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> vecExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers) {
			vecExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return vecExtensions;
	}
private:
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const int WIDTH = 800;
	const int HEIGHT = 600;
#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif
private:
	GLFWwindow* pWindow;
private:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	// Cleaned up with the instance
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	// Automatically created along with the logical device, so also cleaned up with the device
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	// No need to cleanup, will be cleanedup with the swapchain
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
};

int main() {
	HelloTriangleApp app;

	try {
		app.Run();
	}
	catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}