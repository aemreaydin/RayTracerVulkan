#include "DebugHelpers.h"
#include "SetupHelpers.h"
#include <limits>
#include <map>


class HelloTriangleApp
{
public:
	void Run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initWindow()
	{
		glfwInit();
		// Don't create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		pWindow =
			glfwCreateWindow(WIDTH, HEIGHT, "RayTracer Vulkan", nullptr, nullptr);
	}
	void initVulkan()
	{
		createInstance();
		if (enableValidationLayers)
		{
			CDebugHelpers::SetupDebugMessenger(instance, nullptr, &debugMessenger);
		}
		// Create the surface before the physical device
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
	}
	void mainLoop()
	{
		while (!glfwWindowShouldClose(pWindow))
		{
			glfwPollEvents();
		}
	}
	void cleanup()
	{
		for (auto& imageView : swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers)
		{
			CDebugHelpers::DestroyDebugUtilsMessengerEXT(instance, debugMessenger,
														 nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(pWindow);
		glfwTerminate();
	}
private:
	void createInstance()
	{
		// Enable validation layers in Debug
		if (enableValidationLayers && !CSetupHelpers::checkValidationSupport())
		{
			throw std::runtime_error(
				"Validation layers requested, but not supported.");
		}
		// Create the application information before creating the instance
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_1;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Engine";

		auto vecExtensions = CSetupHelpers::getRequiredExtensions();

		uint32_t optionalExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &optionalExtensionCount,
											   nullptr);
		std::vector<VkExtensionProperties> vecExtProperties(optionalExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &optionalExtensionCount,
											   vecExtProperties.data());
		// Check if all the required extensions are supported by Vulkan
		bool isSupported = CSetupHelpers::checkExtensionSupport(
			vecExtensions.data(), static_cast<uint32_t>(vecExtensions.size()),
			vecExtProperties);
		isSupported
			? std::cout << "All the extensions are supported" << std::endl
			: throw std::runtime_error("Some extensions are not supported.");

		// Create the instance information before creating the instance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount =
			static_cast<uint32_t>(vecExtensions.size());
		createInfo.ppEnabledExtensionNames = vecExtensions.data();

		// Create DebugCreateInfo to check for any problems during the creation of
		// the instance
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount =
				static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			CDebugHelpers::PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// Now that we have the VkInstanceCreateInfo, we can create the instance
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create VkInstance.");
		}
	}
	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, pWindow, nullptr, &surface) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the surface.");
		}
	}
	void pickPhysicalDevice()
	{
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		if (physicalDeviceCount == 0)
		{
			throw std::runtime_error("Failed to find a GPU with Vulkan support.");
		}
		std::vector<VkPhysicalDevice> vecDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
								   vecDevices.data());
		for (const auto& device : vecDevices)
		{
			if (CSetupHelpers::isDeviceSuitable(device, surface))
			{
				physicalDevice = device;
				break;
			}
		}
		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU.");
		}
	}
	void createLogicalDevice()
	{
		SQueueFamilyIndices indices =
			CSetupHelpers::findQueueFamilies(physicalDevice, surface);
		float queuePriority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> vecQueueCreateInfos;
		// If the indices are the same, we'll end up using one
		// VkDeviceQueueCreateInfo
		std::set<uint32_t> setQueueFamilyIndices = { indices.graphicsFamily.value(),
													indices.presentFamily.value() };

		for (auto queueFamilyIndex : setQueueFamilyIndices)
		{
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
		deviceCreateInfo.queueCreateInfoCount =
			static_cast<uint32_t>(vecQueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = vecQueueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		// VkDeviceCreateInfo validation layers are no longer used
		// Validation Layers created when creating the VkInstance
		deviceCreateInfo.enabledExtensionCount =
			static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
		deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
		if (enableValidationLayers)
		{
			deviceCreateInfo.enabledLayerCount =
				static_cast<uint32_t>(VALIDATION_LAYERS.size());
			deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		}
		else
		{
			deviceCreateInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device.");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}
	void createSwapChain()
	{
		SSwapChainSupportDetails details =
			CSetupHelpers::querySwapChainSupport(physicalDevice, surface);

		VkSurfaceFormatKHR format =
			CSetupHelpers::chooseSwapSurfaceFormat(details.surfaceFormats);
		VkPresentModeKHR presentMode =
			CSetupHelpers::chooseSwapPresentMode(details.presentModes);
		VkExtent2D extent =
			CSetupHelpers::chooseSwapExtent(details.surfaceCapabilities);

		uint32_t imageCount = details.surfaceCapabilities.minImageCount + 1;
		if (details.surfaceCapabilities.maxImageCount > 0 &&
			imageCount > details.surfaceCapabilities.maxImageCount)
		{
			imageCount = details.surfaceCapabilities.maxImageCount;
		}

		SQueueFamilyIndices indices =
			CSetupHelpers::findQueueFamilies(physicalDevice, surface);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
										 indices.presentFamily.value() };

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		createInfo.preTransform = details.surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the swapchain.");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
								swapChainImages.data());

		swapChainImageFormat = format.format;
		swapChainExtent = extent;
	}
	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		int index = 0;
		for (const auto& image : swapChainImages)
		{
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

			if (vkCreateImageView(device, &createInfo, nullptr,
								  &swapChainImageViews[index++]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create the image view.");
			}
		}
	}

private:
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
	// Automatically created along with the logical device, so also cleaned up
	// with the device
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

int main()
{
	HelloTriangleApp app;

	try
	{
		app.Run();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}