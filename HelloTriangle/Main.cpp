#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <array>
#include <vector>
#include <stdexcept>
#include <functional>


class HelloTriangleApp {
public:
	void Run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
public:
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		if (msgSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			std::cerr << "VALIDATION::VERBOSE: " << pCallbackData->pMessage << std::endl;
		}
		else if (msgSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			std::cerr << "VALIDATION::INFO: " << pCallbackData->pMessage << std::endl;
		}
		else if (msgSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			std::cerr << "VALIDATION::WARNING: " << pCallbackData->pMessage << std::endl;
		}
		else { // ERROR
			std::cerr << "VALIDATION::ERROR: " << pCallbackData->pMessage << std::endl;
		}

		return VK_FALSE;
	}
private:
	void initWindow() {
		glfwInit();
		// Don't create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "RayTracer Vulkan", nullptr, nullptr);
	}
	void initVulkan() {
		createInstance();
		setupDebugMessenger();
	}
	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}
	void cleanup() {
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
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
		bool isSupported = checkExtensionSupport(vecExtensions.data(), vecExtensions.size(), vecExtProperties);
		isSupported ? std::cout << "All the extensions are supported" << std::endl
			: throw std::runtime_error("Some extensions are not supported.");

		// Create the instance information before creating the instance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = vecExtensions.size();
		createInfo.ppEnabledExtensionNames = vecExtensions.data();

		// Create DebugCreateInfo to check for any problems during the creation of the instance
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
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
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed to setup debug messenger.");
		}
	}
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
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
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
		
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func) {
			func(instance, debugMessenger, pAllocator);
		}
	}
private:
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	const int WIDTH = 800;
	const int HEIGHT = 600;
#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif
private:
	GLFWwindow* window;
private:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
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