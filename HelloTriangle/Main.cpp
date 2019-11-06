#include "DebugHelpers.h"
#include "SetupHelpers.h"
#include "ShaderLoader.h"
#include "ModelLoader.h"
#include "GameObject.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <map>
#include <chrono>

const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

std::unique_ptr<CStaticGameObject> upGameObject;

class HelloTriangleApp
{
public:
	HelloTriangleApp() = default;
public:
	void Run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		const auto app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initWindow()
	{
		glfwInit();
		// Don't create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		pWindow =
			glfwCreateWindow(WIDTH, HEIGHT, "RayTracer Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(pWindow, this);
		glfwSetFramebufferSizeCallback(pWindow, framebufferResizeCallback);
	}

	void createScene()
	{
		std::vector<SObjectInformation> vecObjectInformation;
		CModelLoader::GetSceneHierarchy("Models/Scene.json", vecObjectInformation);

		
		for(const auto& model : vecObjectInformation)
		{
			CModelLoader::LoadModel(model, vecVertices, vecIndices);
			upGameObject = std::make_unique<CStaticGameObject>(model);
		}
	}

	void initVulkan()
	{
		createInstance();
		if (enableValidationLayers)
		{
			CDebugHelpers::SetupDebugMessenger(instance, nullptr, &debugMessenger);
		}
		// Create the surface before the physical device
		createScene();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createSwapChainImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createDepthResources();
		createFramebuffers();
		createTextureImage("Models/Chalet/chalet.jpg");
		createTextureImageView();
		createTextureSampler();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(pWindow))
		{
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}

	void cleanup()
	{
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);

		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		auto index = 0;
		for (auto& imageSemaphore : vecSemaphoreImageAvailable)
		{
			vkDestroySemaphore(device, imageSemaphore, nullptr);
			vkDestroySemaphore(device, vecSemaphoreRenderFinished[index], nullptr);
			vkDestroyFence(device, vecInFlightFences[index], nullptr);
			index++;
		}
		for (auto& commandPool : vecCommandPools)
		{
			vkDestroyCommandPool(device, commandPool, nullptr);
		}

		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers)
		{
			CDebugHelpers::DestroyDebugUtilsMessengerExt(instance, debugMessenger,
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
		if (enableValidationLayers && !CSetupHelpers::CheckValidationSupport())
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

		auto vecExtensions = CSetupHelpers::GetRequiredExtensions();

		uint32_t optionalExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &optionalExtensionCount,
		                                       nullptr);
		std::vector<VkExtensionProperties> vecExtProperties(optionalExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &optionalExtensionCount,
		                                       vecExtProperties.data());
		// Check if all the required extensions are supported by Vulkan
		const auto isSupported = CSetupHelpers::CheckExtensionSupport(
			vecExtensions.data(), static_cast<uint32_t>(vecExtensions.size()),
			vecExtProperties);

		if (!isSupported)
		{
			throw std::runtime_error("Extension Support Error.");
		}

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
			createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
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
			if (CSetupHelpers::IsDeviceSuitable(device, surface))
			{
				physicalDevice = device;
				break;
			}
		}
		if (physicalDevice == nullptr)
		{
			throw std::runtime_error("Failed to find a suitable GPU.");
		}
	}

	void createLogicalDevice()
	{
		auto indices =
			CSetupHelpers::FindQueueFamilies(physicalDevice, surface);
		auto queuePriority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> vecQueueCreateInfos;
		// If the indices are the same, we'll end up using one
		// VkDeviceQueueCreateInfo
		std::set<uint32_t> setQueueFamilyIndices = {
			indices.GraphicsFamily.value(),
			indices.PresentFamily.value(),
			indices.TransferFamily.value()
		};

		for (auto queueFamilyIndex : setQueueFamilyIndices)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			vecQueueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

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

		vkGetDeviceQueue(device, indices.GraphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.PresentFamily.value(), 0, &presentQueue);
		vkGetDeviceQueue(device, indices.TransferFamily.value(), 0, &transferQueue);
	}

	void createSwapChain()
	{
		const auto details =
			CSetupHelpers::QuerySwapChainSupport(physicalDevice, surface);

		const auto format =
			CSetupHelpers::ChooseSwapSurfaceFormat(details.SurfaceFormats);
		const auto presentMode =
			CSetupHelpers::ChooseSwapPresentMode(details.PresentModes);
		const auto extent =
			CSetupHelpers::ChooseSwapExtent(pWindow, details.SurfaceCapabilities);

		auto imageCount = details.SurfaceCapabilities.minImageCount + 1;
		if (details.SurfaceCapabilities.maxImageCount > 0 &&
			imageCount > details.SurfaceCapabilities.maxImageCount)
		{
			imageCount = details.SurfaceCapabilities.maxImageCount;
		}

		auto indices =
			CSetupHelpers::FindQueueFamilies(physicalDevice, surface);

		std::set<uint32_t> setQueueFamilyIndices = {
			indices.GraphicsFamily.value(),
			indices.PresentFamily.value(),
			indices.TransferFamily.value()
		};
		std::vector<uint32_t> vecQueueFamilyIndices(setQueueFamilyIndices.begin(),
		                                            setQueueFamilyIndices.end());


		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		//if (indices.GraphicsFamily != indices.PresentFamily)
		//{
		//	createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		//	createInfo.queueFamilyIndexCount = 2;
		//	createInfo.pQueueFamilyIndices = queueFamilyIndices;
		//}
		//else
		//{
		//	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//	createInfo.queueFamilyIndexCount = 0;
		//	createInfo.pQueueFamilyIndices = nullptr;
		//}
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(setQueueFamilyIndices.size());
		createInfo.pQueueFamilyIndices = vecQueueFamilyIndices.data();
		createInfo.preTransform = details.SurfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = nullptr;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the swapchain.");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		vecSwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
		                        vecSwapChainImages.data());

		swapChainImageFormat = format.format;
		swapChainExtent = extent;
	}

	void createSwapChainImageViews()
	{
		vecSwapChainImageViews.resize(vecSwapChainImages.size());
		for (size_t i = 0; i != vecSwapChainImages.size(); ++i)
		{
			vecSwapChainImageViews[i] = createImageView(vecSwapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void createRenderPass()
	{
		// TODO: OWN FUNCTION
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef;
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = CSetupHelpers::FindDepthFormat(physicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef;
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentRef;
		subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		std::array<VkAttachmentDescription, 2> arrAttachmentDescription = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(arrAttachmentDescription.size());
		renderPassCreateInfo.pAttachments = arrAttachmentDescription.data();
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

		if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the render pass.");
		}
	}

	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerBinding = {};
		samplerBinding.binding = 1;
		samplerBinding.descriptorCount = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;		

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerBinding };
		
		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutCreateInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the descriptor set layout.");
		}
	}

	void createGraphicsPipeline()
	{
		std::vector<char> vecVertShaderCode;
		CShaderLoader::ReadShader("Shaders/vShader.spv", vecVertShaderCode);
		std::vector<char> vecFragShaderCode;
		CShaderLoader::ReadShader("Shaders/fShader.spv", vecFragShaderCode);

		const auto vertShaderModule = createShaderModule(vecVertShaderCode);
		const auto fragShaderModule = createShaderModule(vecFragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {};
		vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageCreateInfo.module = vertShaderModule;
		vertShaderStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStateCreateInfo = {};
		fragShaderStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStateCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStateCreateInfo.module = fragShaderModule;
		fragShaderStateCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {
			vertShaderStageCreateInfo,
			fragShaderStateCreateInfo
		};

		const auto bindingDescription = SVertex::GetInputBindingDescription();
		const auto attributeDescriptions = SVertex::GetInputAttributeDescriptions();

		// Vertex Input State
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// Input Assembly State
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewport and Scissors, eventually Viewport state
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.pScissors = &scissor;

		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCreateInfo.lineWidth = 1.0f;
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

		// Multi sampling State
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCreateInfo.minSampleShading = 1.0f;
		multisampleStateCreateInfo.pSampleMask = nullptr;
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		// Color Blend Attachment and Color Blending State
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

		// Dynamic State
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = 2;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Finally the pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
		pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the pipeline layout.");
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.basePipelineHandle = nullptr;
		pipelineCreateInfo.basePipelineIndex = -1;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		pipelineCreateInfo.pTessellationState = nullptr;
		pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS
		)
		{
			throw std::runtime_error("Failed to create the graphics pipeline.");
		}

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	void createFramebuffers()
	{
		vecSwapChainFramebuffers.resize(vecSwapChainImageViews.size());

		for (size_t i = 0; i != vecSwapChainFramebuffers.size(); ++i)
		{
			std::array<VkImageView, 2> arrImageView = {
				vecSwapChainImageViews[i],
				depthImageView
			};

			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.width = swapChainExtent.width;
			framebufferCreateInfo.height = swapChainExtent.height;
			framebufferCreateInfo.layers = 1;
			framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(arrImageView.size());
			framebufferCreateInfo.pAttachments = arrImageView.data();
			framebufferCreateInfo.renderPass = renderPass;

			if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &vecSwapChainFramebuffers[i]) !=
				VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create the framebuffer.");
			}
		}
	}

	void createCommandPool()
	{
		const auto queueFamilyIndices = CSetupHelpers::FindQueueFamilies(physicalDevice, surface);
		vecCommandPools.resize(2);

		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

		// First Command Pool is for the Graphics Queue
		if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &vecCommandPools[0]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the command pool.");
		}

		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.TransferFamily.value();
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		// The second command pool is for the short-lived operations
		if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &vecCommandPools[1]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the command pool");
		}
	}

	void createTextureImage(const std::string& textureDir)
	{
		int width, height, channels;
		const auto pixels = stbi_load(textureDir.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		const auto texSize = static_cast<unsigned long long>(width)* static_cast<unsigned long long>(height) * 4;

		if (!pixels)
		{
			throw std::runtime_error("Failed to load the texture.");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
		             stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, texSize, 0, &data);
		std::memcpy(data, pixels, texSize);
		vkUnmapMemory(device, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(width, height,
		            VK_FORMAT_R8G8B8A8_UNORM,
		            VK_IMAGE_TILING_OPTIMAL,
		            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		            textureImage,
		            textureImageMemory);

		transitionImageLayout(textureImage,
		                      VK_FORMAT_R8G8B8A8_UNORM,
		                      VK_IMAGE_LAYOUT_UNDEFINED,
		                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyBufferToImage(stagingBuffer, textureImage, width, height);

		transitionImageLayout(textureImage,
		                      VK_FORMAT_R8G8B8A8_UNORM,
		                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createTextureImageView()
	{
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void createTextureSampler()
	{
		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = 16;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;

		if(vkCreateSampler(device, &createInfo, nullptr, &textureSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the sampler.");
		}
	}

	void createVertexBuffer()
	{
		const auto bufferSize = sizeof(vecVertices[0]) * vecVertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize,
		             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		             stagingBuffer,
		             stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		std::memcpy(data, vecVertices.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize,
		             VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		             vertexBuffer,
		             vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		// Cleanup staging buffer and memory
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffer()
	{
		const auto bufferSize = sizeof(vecIndices[0]) * vecIndices.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize,
		             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		             stagingBuffer,
		             stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		std::memcpy(data, vecIndices.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize,
		             VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		             indexBuffer,
		             indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createDepthResources()
	{
		const auto depthFormat = CSetupHelpers::FindDepthFormat(physicalDevice);

		createImage(swapChainExtent.width,
					swapChainExtent.height,
					depthFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, 
					depthImageMemory);

		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		//transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void createCommandBuffers()
	{
		vecCommandBuffers.resize(vecSwapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = vecCommandPools[0];
		allocateInfo.commandBufferCount = static_cast<uint32_t>(vecCommandBuffers.size());
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(device, &allocateInfo, vecCommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the command buffers.");
		}

		std::array<VkClearValue, 2> arrClearValue = {};
		arrClearValue[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		arrClearValue[1].depthStencil = { 1.0f, 0 };
		
		for (size_t i = 0; i != vecCommandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(vecCommandBuffers[i], &commandBufferBeginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to begin recording the command buffer.");
			}

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = vecSwapChainFramebuffers[i];
			renderPassBeginInfo.renderArea.offset = {0, 0};
			renderPassBeginInfo.renderArea.extent = swapChainExtent;
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(arrClearValue.size());
			renderPassBeginInfo.pClearValues = arrClearValue.data();
			// Begin Render Pass
			vkCmdBeginRenderPass(vecCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			// Bind the Graphics Pipeline
			vkCmdBindPipeline(vecCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			// Draw
			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(vecCommandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(vecCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(vecCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
			                        &vecDescriptorSet[i], 0, nullptr);
			// Now using indices to draw
			vkCmdDrawIndexed(vecCommandBuffers[i], static_cast<uint32_t>(vecIndices.size()), 1, 0, 0, 0);
			//vkCmdDraw(commandBuffer, vecVertices.size(), 1, 0, 0);
			// End Render Pass
			vkCmdEndRenderPass(vecCommandBuffers[i]);
			// End recording the command buffer
			if (vkEndCommandBuffer(vecCommandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to record command buffer end.");
			}
		}
	}

	void createSyncObjects()
	{
		vecSemaphoreImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
		vecSemaphoreRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
		vecInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		auto index = 0;
		for (auto& semaphoreImage : vecSemaphoreImageAvailable)
		{
			if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreImage) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &vecSemaphoreRenderFinished[index]) !=
				VK_SUCCESS ||
				vkCreateFence(device, &fenceCreateInfo, nullptr, &vecInFlightFences[index]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create on or both of the semaphores.");
			}
			index++;
		}
	}

	void createUniformBuffers()
	{
		const auto uboSize = sizeof(SUniformBufferObject);

		vecUniformBuffer.resize(vecSwapChainImages.size());
		vecUniformBufferMemory.resize(vecSwapChainImages.size());

		for (size_t i = 0; i < vecUniformBuffer.size(); ++i)
		{
			createBuffer(uboSize,
			             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			             vecUniformBuffer[i], vecUniformBufferMemory[i]);
		}
	}

	void createDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> vecDescriptorPoolSize = {};
		vecDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vecDescriptorPoolSize[0].descriptorCount = static_cast<uint32_t>(vecSwapChainImages.size());;
		vecDescriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vecDescriptorPoolSize[1].descriptorCount = static_cast<uint32_t>(vecSwapChainImages.size());;

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.maxSets = static_cast<uint32_t>(vecSwapChainImages.size());
		createInfo.poolSizeCount = static_cast<uint32_t>(vecDescriptorPoolSize.size());
		createInfo.pPoolSizes = vecDescriptorPoolSize.data();

		if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the descriptor pool.");
		}
	}

	void createDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> vecDescriptorSetLayout(vecSwapChainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = static_cast<uint32_t>(vecDescriptorSetLayout.size());
		allocateInfo.pSetLayouts = vecDescriptorSetLayout.data();

		vecDescriptorSet.resize(vecSwapChainImages.size());
		if (vkAllocateDescriptorSets(device, &allocateInfo, vecDescriptorSet.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate descriptor sets.");
		}

		for (size_t i = 0; i != vecSwapChainImages.size(); ++i)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = vecUniformBuffer[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(SUniformBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> vecWriteDescriptorSet = {};
			vecWriteDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vecWriteDescriptorSet[0].descriptorCount = 1;
			vecWriteDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			vecWriteDescriptorSet[0].dstSet = vecDescriptorSet[i];
			vecWriteDescriptorSet[0].dstBinding = 0;
			vecWriteDescriptorSet[0].dstArrayElement = 0;
			vecWriteDescriptorSet[0].pBufferInfo = &bufferInfo;
			vecWriteDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vecWriteDescriptorSet[1].descriptorCount = 1;
			vecWriteDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			vecWriteDescriptorSet[1].dstSet = vecDescriptorSet[i];
			vecWriteDescriptorSet[1].dstBinding = 1;
			vecWriteDescriptorSet[1].dstArrayElement = 0;
			vecWriteDescriptorSet[1].pImageInfo = &imageInfo;
			
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(vecWriteDescriptorSet.size()), vecWriteDescriptorSet.data(), 0, nullptr);
		}
	}

	[[nodiscard]] VkImageView createImageView(const VkImage image, const VkFormat& format, const VkImageAspectFlags aspectFlags) const
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the image view.");
		}

		return imageView;
	}

	void updateUniformBuffer(const uint32_t imageIndex)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		const auto currentTime = std::chrono::high_resolution_clock::now();

		const auto deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).
			count();

		SUniformBufferObject ubo = {};
		ubo.Model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.Projection = glm::perspective(glm::radians(45.0f),
		                                    swapChainExtent.width / static_cast<float>(swapChainExtent.height), 
											0.1f,
		                                    100.f);
		ubo.Projection[1][1] *= -1; // Y is inverted compared to OpenGL

		void* data;
		vkMapMemory(device, vecUniformBufferMemory[imageIndex], 0, sizeof(ubo), 0, &data);
		std::memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, vecUniformBufferMemory[imageIndex]);
	}

	void drawFrame()
	{
		// Wait for the frame to be finished
		vkWaitForFences(device, 1, &vecInFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		// Acquire an image from the swapchain
		// As in, acquire the index that refers to the VkImage in vecSwapchainImages
		uint32_t imageIndex;
		auto result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(),
		                                    vecSemaphoreImageAvailable[currentFrame], nullptr, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire the swapchain image");
		}

		updateUniformBuffer(imageIndex);

		VkSubmitInfo submitInfo = {};
		VkSemaphore waitSemaphores[] = {
			vecSemaphoreImageAvailable[currentFrame]
		};
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		VkSemaphore signalSemaphores[] = {
			vecSemaphoreRenderFinished[currentFrame]
		};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vecCommandBuffers[imageIndex];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &vecInFlightFences[currentFrame]);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, vecInFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit the queue.");
		}
		// Presentation
		VkPresentInfoKHR presentInfo = {};
		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present the swap chain image.");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void recreateSwapChain()
	{
		auto width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(pWindow, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createSwapChainImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createDepthResources();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}

	void cleanupSwapChain()
	{
		for (auto& framebuffer : vecSwapChainFramebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyImage(device, depthImage, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);
		vkDestroyImageView(device, depthImageView, nullptr);

		vkFreeCommandBuffers(device, vecCommandPools[0], static_cast<uint32_t>(vecCommandBuffers.size()),
		                     vecCommandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		for (auto imageView : vecSwapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);

		for (size_t i = 0; i < vecUniformBuffer.size(); ++i)
		{
			vkDestroyBuffer(device, vecUniformBuffer[i], nullptr);
			vkFreeMemory(device, vecUniformBufferMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}

	void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usageFlags,
	                  const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
	{
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the buffer.");
		}

		// Get Memory Requirements
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = CSetupHelpers::FindMemoryType(
			physicalDevice, memoryRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate memory for the buffer.");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void createImage(const uint32_t width, const uint32_t height, const VkFormat format, const VkImageTiling tiling,
	                 const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, VkImage& image,
	                 VkDeviceMemory& imageMemory) const
	{
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.usage = usage;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		vkCreateImage(device, &imageCreateInfo, nullptr, &image);

		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements(device, image, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = CSetupHelpers::FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits,
		                                                             properties);

		if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate memory for the image.");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void copyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size)
	{
		const auto commandBuffer = CreateBeginCommandBuffer(1);
		// Copy vertex buffer using command buffer
		VkBufferCopy bufferCopy;
		bufferCopy.size = size;
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset = 0;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
		// End command buffer
		EndCommandBuffer(commandBuffer, 1);
	}

	void transitionImageLayout(const VkImage image, const VkFormat format, const VkImageLayout oldLayout,
	                           const VkImageLayout newLayout)
	{
		const auto commandBuffer = CreateBeginCommandBuffer(0);


		VkImageMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.oldLayout = oldLayout;
		memoryBarrier.newLayout = newLayout;
		memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image = image;
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memoryBarrier.subresourceRange.baseMipLevel = 0;
		memoryBarrier.subresourceRange.levelCount = 1;
		memoryBarrier.subresourceRange.baseArrayLayer = 0;
		memoryBarrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = 0;
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
				 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
				 newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = 0;
			memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			throw std::invalid_argument("Unsupported layout transition.");
		}

		vkCmdPipelineBarrier(commandBuffer,
		                     sourceStage, destinationStage,
		                     0, 0,
		                     nullptr, 0,
		                     nullptr, 1,
		                     &memoryBarrier);

		EndCommandBuffer(commandBuffer, 0);
	}

	void copyBufferToImage(const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height)
	{
		// For operations that end up in VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT USE GRAPHICS QUEUE
		const auto commandBuffer = CreateBeginCommandBuffer(0);

		VkBufferImageCopy bufferImageCopy = {};
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageOffset = {0, 0, 0};
		bufferImageCopy.imageExtent.width = width;
		bufferImageCopy.imageExtent.height = height;
		bufferImageCopy.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);

		EndCommandBuffer(commandBuffer, 0);
	}

	[[nodiscard]] VkCommandBuffer CreateBeginCommandBuffer(const uint32_t queueIndex)
	{
		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = vecCommandPools[queueIndex];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void EndCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t queueIndex)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queueIndex == 0 ? graphicsQueue : transferQueue, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(queueIndex == 0 ? graphicsQueue : transferQueue);
		vkFreeCommandBuffers(device, vecCommandPools[queueIndex], 1, &commandBuffer);
	}

	[[nodiscard]] VkShaderModule createShaderModule(const std::vector<char>& code) const
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the shader module.");
		}
		return shaderModule;
	}

private:
#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif
private:
	GLFWwindow* pWindow = nullptr;
private:
	VkInstance instance = nullptr;
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	// Cleaned up with the instance
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	// Automatically created along with the logical device, so also cleaned up
	// with the device
	VkQueue graphicsQueue = nullptr;
	VkQueue presentQueue = nullptr;
	VkQueue transferQueue = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapChain = nullptr;
	// No need to cleanup, will be cleaned up with the swap chain
	std::vector<VkImage> vecSwapChainImages;
	std::vector<VkImageView> vecSwapChainImageViews;
	VkFormat swapChainImageFormat = {};
	VkExtent2D swapChainExtent = {};
	VkRenderPass renderPass = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorPool descriptorPool = nullptr;
	// No need to cleanup, will be cleaned up with the descriptor pool
	std::vector<VkDescriptorSet> vecDescriptorSet;
	VkPipeline graphicsPipeline = nullptr;
	std::vector<VkFramebuffer> vecSwapChainFramebuffers;
	std::vector<VkCommandPool> vecCommandPools;
	// No need to cleanup, will be cleaned up with the command pool
	std::vector<VkCommandBuffer> vecCommandBuffers;
	std::vector<VkSemaphore> vecSemaphoreImageAvailable;
	std::vector<VkSemaphore> vecSemaphoreRenderFinished;
	std::vector<VkFence> vecInFlightFences;
	size_t currentFrame = 0;
	bool framebufferResized = false;
	std::vector<SVertex> vecVertices;
	std::vector<uint32_t> vecIndices;
	VkImage textureImage = nullptr;
	VkDeviceMemory textureImageMemory = nullptr;
	VkImageView textureImageView = nullptr;
	VkSampler textureSampler = nullptr;
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;
	VkImage depthImage = nullptr;
	VkDeviceMemory depthImageMemory = nullptr;
	VkImageView depthImageView = nullptr;
	std::vector<VkBuffer> vecUniformBuffer;
	std::vector<VkDeviceMemory> vecUniformBufferMemory;
};

int main()
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	HelloTriangleApp app;

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
