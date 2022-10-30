/*
*	Desc: Vulkan parts builder/utilities
*	Note: This just builds boilerplate stuff in vulkan, it's a lot of code....
*/
#pragma once
#include <cstdint>
#include <vector>
#include <functional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "side-builders.hpp"
#include "../types/vtypes.hpp"

#include "../../../../../utilities/utilFlags.hpp"
#include "../../../../../utilities/console/err.hpp"
#include "../../../../../utilities/console/logger.hpp"

namespace vulkan
{
	static auto createInstanceAndSurface(GLFWwindow* window)
	{
		// Basic app information

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = utils::applicationName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = utils::mainContextName;
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Extentions

		std::uint32_t glfwExtensionCount;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions;
		for (auto i = 0u; i < glfwExtensionCount; i++)
			extensions.push_back(glfwExtensions[i]);

		if constexpr (utils::vulkanDbg)
			extensions.push_back(utils::vulkanDebugLayerName);

		std::uint32_t extensionCount{ 0u };
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		if (!extensionCount)
			throw err::err("no extention supported");

		// Group to create instance information

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if constexpr (utils::vulkanDbg)
		{
			createInfo.enabledLayerCount = 1;
			createInfo.ppEnabledLayerNames = &utils::vulkanDebugLayerName;
		}

		// Create instance

		VkInstance instance;
		if (!vkCreateInstance(&createInfo, nullptr, &instance))
			throw err::err("could not create vulkan instance");

		// Make window surface

		VkSurfaceKHR windowSurface;
		if (!glfwCreateWindowSurface(instance, window, NULL, &windowSurface))
			throw err::err("can't create window surface");

		return std::make_tuple(instance, windowSurface);
	}

	static auto findPhysicalDevice(VkInstance instance)
	{
		VkPhysicalDevice physicalDevice;

		// Find devices that are supported by vulkan

		auto deviceCount{ 1u };
		VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevice);

		if (res != VK_SUCCESS && res != VK_INCOMPLETE)
			throw err::err("failded to enumerate devices");

		if (!deviceCount)
			throw err::err("no devices found that support vulkan");

		// Later check if device vulkan supported version is up-to-date
		// ...

		// Check for extension support

		auto extensionCount{ 0u };
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		if (!extensionCount)
			throw err::err("your device supports no extensions");

		// Check for swapchain support

		std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

		for (const auto& extension : deviceExtensions)
			if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) != 0)
				throw err::err("your device doesn't support swapchain");

		if constexpr (utils::vulkanDbg)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

			logger.log("Current device \"%s\"", deviceProperties.deviceName);
		}

		return physicalDevice;
	}

	static auto getQueueIndexes(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
	{
		// Check family queues count

		auto familyQueuesCount{ 0u };
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyQueuesCount, nullptr);

		if (!familyQueuesCount)
			throw err::err("your device has 0 family queues");

		// Now write these family queues to a vector
		// Note: Make sure to check for present & graphics queues primarily 

		std::vector<VkQueueFamilyProperties> queueFamilies(familyQueuesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyQueuesCount, queueFamilies.data());

		std::uint32_t graphicsQueueIndex{ -1u };
		std::uint32_t presentQueueIndex{ -1u };

		for (auto i = 0u; i < familyQueuesCount; i++)
		{
			auto presentSupport{ 0u };
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

			const auto queueProperties = queueFamilies[i];

			if (queueProperties.queueCount > 0 && queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				// Set graphics queue

				graphicsQueueIndex = i;

				// Check if present queue is found here

				if (presentSupport) {
					presentQueueIndex = i;
					break;
				}
			}

			// Just set present if graphics was not found
			if (graphicsQueueIndex == -1 && presentSupport)
			{
				presentQueueIndex = i;
			}
		}

		if (graphicsQueueIndex == -1 || presentQueueIndex == -1)
			throw err::err("one of the queues were not found");
		return std::make_tuple(graphicsQueueIndex, presentQueueIndex);
	}

	static auto createLogicalDevices(VkPhysicalDevice physicalDevice, std::tuple<std::uint32_t, std::uintptr_t>& queueIndexes)
	{
		// Make queue create information

		VkDeviceQueueCreateInfo queueCreateInfo[2] = {};
		float queuePriority = 1.0f;


		queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[0].queueFamilyIndex = std::get<0>(queueIndexes);
		queueCreateInfo[0].queueCount = 1;
		queueCreateInfo[0].pQueuePriorities = &queuePriority;

		queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[0].queueFamilyIndex = std::get<1>(queueIndexes);
		queueCreateInfo[0].queueCount = 1;
		queueCreateInfo[0].pQueuePriorities = &queuePriority;

		// Create device information

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

		if (std::get<0>(queueIndexes) == std::get<01>(queueIndexes))
			deviceCreateInfo.queueCreateInfoCount = 1;
		else
			deviceCreateInfo.queueCreateInfoCount = 2;

		// Some shader settings & extention setup

		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.shaderClipDistance = VK_TRUE;
		enabledFeatures.shaderCullDistance = VK_TRUE;

		const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		if constexpr (utils::vulkanDbg) {
			deviceCreateInfo.enabledLayerCount = 1;
			deviceCreateInfo.ppEnabledLayerNames = &utils::vulkanDebugLayerName;
		}

		// Finally create the logical device

		VkDevice device;
		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
			throw err::err("cannot create logical device");

		// Get memory properties
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

		return std::make_tuple(device, deviceMemoryProperties);
	}

	static auto getQueues(VkDevice device, std::tuple<std::uint32_t, std::uintptr_t>& queueIndexes)
	{
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		vkGetDeviceQueue(device, std::get<0>(queueIndexes), 0, &graphicsQueue);
		vkGetDeviceQueue(device, std::get<1>(queueIndexes), 0, &presentQueue);

		return std::make_tuple(graphicsQueue, presentQueue);
	}

	[[maybe_unused]]
	static auto passDebuggerFunc(VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunc)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.pfnCallback = callbackFunc;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
		VkDebugReportCallbackEXT callback;

		if (CreateDebugReportCallback(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
			throw err::err("cannot create debug pass");

		return callback;
	}

	static auto createSemaphores(VkDevice device)
	{
		// Create information shared between both semaphores

		VkSemaphoreCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Create image available semaphore

		VkSemaphore imageAvailableSemaphore;
		if (vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS)
			throw err::err("cannot create ImageAvailable semaphore");

		// Create rendering semaphore

		VkSemaphore renderingFinishedSemaphore;
		if (vkCreateSemaphore(device, &createInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS)
			throw err::err("cannot create RenderingFinished semaphore");


		return std::make_tuple(imageAvailableSemaphore, renderingFinishedSemaphore);
	}

	static auto createCommandPool(VkDevice device, std::uint32_t graphicsQueueIndex)
	{
		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = graphicsQueueIndex;

		VkCommandPool commandPool;
		if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw err::err("cannot create command pool");

		return commandPool;
	}

	static auto createVertexBuffer(std::tuple<VkDevice, VkPhysicalDeviceMemoryProperties> deviceSet, VkQueue graphicsQueue, VkCommandPool commandPool)
	{
		// Quick definition
		const auto device = std::get<0>(deviceSet);
		const auto deviceMemoryProps = std::get<1>(deviceSet);

		// Creating simple vertice information

		types::Vec<types::Vertex> vertices =
		{
			{ { -0.5f, -0.5f,  0.0f }, {  { 1.0f, 0.0f, 0.0f } } },
			{ { -0.5f,  0.5f,  0.0f }, {  { 1.0f, 0.0f, 0.0f } } },
			{ {  0.5f,  0.5f,  0.0f }, {  { 1.0f, 0.0f, 0.0f } } }
		};

		std::vector<uint32_t> indices = { 0, 1, 2 };

		auto verticesSize = static_cast<std::uint32_t>(vertices.size() * sizeof(vertices[0]));
		auto indicesSize = static_cast<std::uint32_t>(indices.size() * sizeof(indices[0]));

		// We allocate memory & create structure for simple "Stage buffers"

		VkMemoryAllocateInfo memoryAllocation{ };
		memoryAllocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements memoryRequirements{ };

		struct {
			struct StageBuffer
			{
				VkDeviceMemory memory;
				VkBuffer buffer;
			};

			StageBuffer vertices;
			StageBuffer indices;
		} *stageBuffers;


		// We create our command buffer

		VkCommandBufferAllocateInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufInfo.commandPool = commandPool;
		cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufInfo.commandBufferCount = 1;

		VkCommandBuffer copyCommandBuffer;
		vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer);

		// Buffer handling

		struct VertexBuffer
		{
			VkBuffer buffer;
			VkDeviceMemory memory;

			VkBuffer index;
			VkDeviceMemory indexMemory;
		} *vertexBuffer;

		void* tempData;

		// Handle vertices stage buffer

		std::function<void()> createVertices = [&] {
			// Create buffer on the CPU

			VkBufferCreateInfo vertexBufferInfo = {};
			vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferInfo.size = verticesSize;
			vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stageBuffers->vertices.buffer);

			// Set up buffer to host

			vkGetBufferMemoryRequirements(device, stageBuffers->vertices.buffer, &memoryRequirements);
			memoryAllocation.allocationSize = memoryRequirements.size;
			getMemoryType(deviceMemoryProps, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memoryAllocation.memoryTypeIndex);
			vkAllocateMemory(device, &memoryAllocation, nullptr, &stageBuffers->vertices.memory);

			vkMapMemory(device, stageBuffers->vertices.memory, 0, verticesSize, 0, &tempData);
			std::memcpy(tempData, vertices.data(), verticesSize);
			vkUnmapMemory(device, stageBuffers->vertices.memory);
			vkBindBufferMemory(device, stageBuffers->vertices.buffer, stageBuffers->vertices.memory, 0);

			// Now setup this buffer within the GPU

			vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertexBuffer->buffer);
			vkGetBufferMemoryRequirements(device, vertexBuffer->buffer, &memoryRequirements);
			memoryAllocation.allocationSize = memoryRequirements.size;
			getMemoryType(deviceMemoryProps, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocation.memoryTypeIndex);

			vkAllocateMemory(device, &memoryAllocation, nullptr, &vertexBuffer->memory);
			vkBindBufferMemory(device, vertexBuffer->buffer, vertexBuffer->memory, 0);
		};

		// Handle indicies stage buffer

		std::function<void()> createIndicies = [&] {
			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(copyCommandBuffer, &bufferBeginInfo);

			VkBufferCopy copyRegion = {};
			copyRegion.size = verticesSize;
			vkCmdCopyBuffer(copyCommandBuffer, stageBuffers->vertices.buffer, vertexBuffer->buffer, 1, &copyRegion);
			copyRegion.size = indicesSize;
			vkCmdCopyBuffer(copyCommandBuffer, stageBuffers->indices.buffer, vertexBuffer->index, 1, &copyRegion);

			vkEndCommandBuffer(copyCommandBuffer);
		};

		createVertices();
		createIndicies();


		// Submit to queue

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCommandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &copyCommandBuffer);

		vkDestroyBuffer(device, stageBuffers->vertices.buffer, nullptr);
		vkFreeMemory(device, stageBuffers->vertices.memory, nullptr);
		vkDestroyBuffer(device, stageBuffers->indices.buffer, nullptr);
		vkFreeMemory(device, stageBuffers->indices.memory, nullptr);

		auto vertexDescriptor(new types::VertexInputBindingDescriptors);

		// Binding and attribute descriptions
		vertexDescriptor->main.binding = 0;
		vertexDescriptor->main.stride = sizeof(vertices[0]);
		vertexDescriptor->main.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		vertexDescriptor->attributes.resize(2);
		vertexDescriptor->attributes[0].binding = 0;
		vertexDescriptor->attributes[0].location = 0;
		vertexDescriptor->attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;

		vertexDescriptor->attributes[1].binding = 0;
		vertexDescriptor->attributes[1].location = 1;
		vertexDescriptor->attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexDescriptor->attributes[1].offset = sizeof(float) * 3;

		return std::make_tuple(vertexBuffer, vertexDescriptor);
	}

	static auto createUniformBuffer(std::tuple<VkDevice, VkPhysicalDeviceMemoryProperties> deviceSet)
	{

		// Quick definition

		const auto device = std::get<0>(deviceSet);
		const auto deviceMemoryProps = std::get<1>(deviceSet);

		// Create a uniform buffer

		struct
		{
			VkBuffer buffer;
			VkDeviceMemory memory;
			types::Matrix<double, 3u> transformationMatrix;
		} *uniformBuffer;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(uniformBuffer->transformationMatrix);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer->buffer);

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, uniformBuffer->buffer, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		getMemoryType(deviceMemoryProps, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);

		vkAllocateMemory(device, &allocInfo, nullptr, &uniformBuffer->memory);
		vkBindBufferMemory(device, uniformBuffer->buffer, uniformBuffer->memory, 0);

		return uniformBuffer;
	}

	static auto createSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, VkSwapchainKHR oldSwapchain = nullptr)
	{
		// e

		types::SwapchainInformation swapchainInformation;

		// Get the surface capabilities

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities) != VK_SUCCESS)
			throw err::err("failed to aquire capabilities");

		// Fetch the supported format counts

		auto formatCount{ 0u };
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0u)
			throw err::err("failed get supported format count");

		// Get surface formats

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS)
			throw err::err("failed get surface formats");

		// Find supported present modes

		auto presentModeCount{ 0u };
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0)
			throw err::err("can't get count of present modes");

		// Get list of present modes

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
			throw err::err("failed to get list of present modes");

		// Determine number of images for swap chain

		auto imageCount = surfaceCapabilities.minImageCount++;
		if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount)
			imageCount = surfaceCapabilities.maxImageCount;

		// Select a surface format

		VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);

		// Select swap chain size

		swapchainInformation.extent = chooseSwapExtent(surfaceCapabilities);

		// Determine transformation to use (preferring no transform)

		VkSurfaceTransformFlagBitsKHR surfaceTransform;
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		else
			surfaceCapabilities.currentTransform;

		VkPresentModeKHR presentMode = choosePresentMode(presentModes);

		// Finally, create the swap chain

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = windowSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = swapchainInformation.extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = surfaceTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapchain;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchainInformation.swapchain) != VK_SUCCESS)
			throw err::err("failed to create swapchain!");

		if (oldSwapchain != nullptr)
			vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

		oldSwapchain = swapchainInformation.swapchain;
		VkFormat swapChainFormat = surfaceFormat.format;

		auto actualImageCount = 0u;
		if (vkGetSwapchainImagesKHR(device, swapchainInformation.swapchain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0)
			throw err::err("failed to get number of swapchain images");

		swapchainInformation.images.resize(actualImageCount);

		if (vkGetSwapchainImagesKHR(device, swapchainInformation.swapchain, &actualImageCount, swapchainInformation.images.data()) != VK_SUCCESS)
			throw err::err("cannot get swapchain images");

		return swapchainInformation;
	}

	static auto makeRenderPass(VkDevice device, VkFormat swapChainFormat)
	{
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = swapChainFormat;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subPassDescription = {};
		subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPassDescription.colorAttachmentCount = 1;
		subPassDescription.pColorAttachments = &colorAttachmentReference;

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachmentDescription;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDescription;

		VkRenderPass renderPass;
		if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
			throw err::err("can't make a render pass");
		return renderPass;
	}

	static auto createImageViews(VkDevice device, VkFormat swapChainFormat, types::Vec<VkImage>& swapChainImages)
	{
		std::vector<VkImageView> swapChainImageViews;
		swapChainImageViews.resize(swapChainImages.size());

		// Create an image view for every image in the swap chain
		for (auto i = 0u; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
				throw err::err("failed to create an image view, index {}", i);
		}

		return swapChainImageViews;
	}

	static auto createFrameBuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView> swapChainImageViews, types::SwapchainInformation swapchainInformation)
	{
		std::vector<VkFramebuffer> swapChainFramebuffers;
		swapChainFramebuffers.resize(swapchainInformation.images.size());

		for (auto i = 0u; i < swapchainInformation.images.size(); i++) {
			VkFramebufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = renderPass;
			createInfo.attachmentCount = 1;
			createInfo.pAttachments = &swapChainImageViews[i];
			createInfo.width = swapchainInformation.extent.width;
			createInfo.height = swapchainInformation.extent.height;
			createInfo.layers = 1;

			if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw err::err("failed to create a frame buffer at {}", i);
			}
		}

		return swapChainFramebuffers;
	}

	static auto makeGraphicsPass()
	{

	}
}