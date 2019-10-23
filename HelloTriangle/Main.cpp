#include "DebugHelpers.h"
#include "SetupHelpers.h"
#include "ShaderLoader.h"
#include <shaderc/shaderc.h>
#include <map>

const int MAX_FRAMES_IN_FLIGHT = 2;


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
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
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
		auto index = 0;
		for(auto& imageSemaphore : vecSemaphoreImageAvailable)
		{
			vkDestroySemaphore(device, imageSemaphore, nullptr);
			vkDestroySemaphore(device, vecSemaphoreRenderFinished[index], nullptr);
			vkDestroyFence(device, vecInFlightFences[index], nullptr);
			index++;
		}
		vkDestroyCommandPool(device, commandPool, nullptr);
		
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
			indices.PresentFamily.value()
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
		uint32_t queueFamilyIndices[] = {
			indices.GraphicsFamily.value(),
			indices.PresentFamily.value()
		};

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (indices.GraphicsFamily != indices.PresentFamily)
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

	void createImageViews()
	{
		vecSwapChainImageViews.resize(vecSwapChainImages.size());
		auto index = 0;
		for (const auto& image : vecSwapChainImages)
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
			                      &vecSwapChainImageViews[index++]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create the image view.");
			}
		}
	}

	void createRenderPass()
	{
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

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

		if(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the render pass.");
		}
	}

	void createGraphicsPipeline()
	{
		const auto vShader = CShaderLoader::ReadShader("Shaders/vShader.spv");
		const auto fShader = CShaderLoader::ReadShader("Shaders/fShader.spv");

		const auto vertShaderModule = createShaderModule(vShader);
		const auto fragShaderModule = createShaderModule(fShader);

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

		// Vertex Input State
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;

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
		scissor.offset = { 0, 0 };
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
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		pipelineCreateInfo.pTessellationState = nullptr;
		pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.subpass = 0;

		if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the graphics pipeline.");
		}

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	void createFramebuffers()
	{
		vecSwapChainFramebuffers.resize(vecSwapChainImageViews.size());
		auto index = 0;
		for(const auto& imageView : vecSwapChainImageViews)
		{
			VkImageView attachments[] = {
				imageView
			};

			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.width = swapChainExtent.width;
			framebufferCreateInfo.height = swapChainExtent.height;
			framebufferCreateInfo.layers = 1;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.renderPass = renderPass;

			if(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &vecSwapChainFramebuffers[index++]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create the framebuffer.");
			}
		}
	}

	void createCommandPool()
	{
		const auto queueFamilyIndices = CSetupHelpers::FindQueueFamilies(physicalDevice, surface);

		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

		if(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the command pool.");
		}
	}

	void createCommandBuffers()
	{
		vecCommandBuffers.resize(vecSwapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = commandPool;
		allocateInfo.commandBufferCount = static_cast<uint32_t>(vecCommandBuffers.size());
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(device, &allocateInfo, vecCommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the command buffers.");
		}

		auto index = 0;
		for(auto& commandBuffer : vecCommandBuffers)
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			
			if(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to begin recording the command buffer.");
			}

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = vecSwapChainFramebuffers[index++];
			renderPassBeginInfo.renderArea.offset = { 0,0 };
			renderPassBeginInfo.renderArea.extent = swapChainExtent;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearValue;
			// Begin Render Pass
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			// Bind the Graphics Pipeline
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			// Draw
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			// End Render Pass
			vkCmdEndRenderPass(commandBuffer);
			// End recording the command buffer
			if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
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
		for(auto& semaphoreImage: vecSemaphoreImageAvailable)
		{			
			if(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreImage) != VK_SUCCESS ||
			   vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &vecSemaphoreRenderFinished[index]) != VK_SUCCESS ||
			   vkCreateFence(device, &fenceCreateInfo, nullptr, &vecInFlightFences[index]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create on or both of the semaphores.");
			}
			index++;
		}
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

		if(result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire the swapchain image");
		}
		
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
		
		if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, vecInFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit the queue.");
		}
		// Presentation
		VkPresentInfoKHR presentInfo = {};
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present the swap chain image.");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void recreateSwapChain()
	{
		int width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(pWindow, &width, &height);
			glfwWaitEvents();
		}
		
		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	void cleanupSwapChain()
	{
		for (auto framebuffer : vecSwapChainFramebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(vecCommandBuffers.size()), vecCommandBuffers.data());
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		for (auto imageView : vecSwapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
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
	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapChain = nullptr;
	// No need to cleanup, will be cleaned up with the swap chain
	std::vector<VkImage> vecSwapChainImages;
	std::vector<VkImageView> vecSwapChainImageViews;
	VkFormat swapChainImageFormat = {};
	VkExtent2D swapChainExtent = {};
	VkRenderPass renderPass = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	std::vector<VkFramebuffer> vecSwapChainFramebuffers;
	VkCommandPool commandPool = nullptr;
	// No need to cleanup, will be cleaned up with the command pool
	std::vector<VkCommandBuffer> vecCommandBuffers;
	std::vector<VkSemaphore> vecSemaphoreImageAvailable;
	std::vector<VkSemaphore> vecSemaphoreRenderFinished;
	std::vector<VkFence> vecInFlightFences;
	size_t currentFrame = 0;
	bool framebufferResized = false;
};

int main()
{
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
