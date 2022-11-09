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
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = utils::applicationName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = utils::mainContextName;
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Get extentions to put on the vulkan instance

		unsigned int glfwExtensionCount;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions;
		for (auto i = 0u; i < glfwExtensionCount; i++)
			extensions.push_back(glfwExtensions[i]);

	//	extensions.push_back("VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME");

		if constexpr(utils::vulkanDbg)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		// Check for extensions

		auto extensionCount{ 0u };
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		if (extensionCount == 0)
			throw err::err("no extentions supported");

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.enabledLayerCount = 0;

		if constexpr(utils::vulkanDbg) {
			createInfo.enabledLayerCount = static_cast<std::uint32_t>(utils::vulkanDebugLayerName.size());
			createInfo.ppEnabledLayerNames = utils::vulkanDebugLayerName.data();
		}

		// Initialize Vulkan instance

		VkInstance instance;
		auto res = vkCreateInstance(&createInfo, nullptr, &instance);
		if (res != VK_SUCCESS)
			throw err::err("can't make vulkan instance");

		// Make window surface

		VkSurfaceKHR windowSurface;
		 res = glfwCreateWindowSurface(instance, window, NULL, &windowSurface);
		if (res != VK_SUCCESS)
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

		auto supportsSwapchain{ false };
		for (const auto& extension : deviceExtensions)
			if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) != 0)
				supportsSwapchain = true;

		if(!supportsSwapchain)
			throw err::err("your device doesn't support swapchain");

		if constexpr (utils::vulkanDbg)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

			logger.log("Current device \"%s\"\n", deviceProperties.deviceName);
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

		std::optional<std::uint32_t> graphicsQueueIndex{ std::nullopt };
		std::optional<std::uint32_t> presentQueueIndex{ std::nullopt };

		for (auto i = 0u; i < familyQueuesCount; i++)
		{
			std::uint32_t presentSupport{ 0u };
			if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport) != VK_SUCCESS)
				throw err::err("failed to get device support");

			auto queueProperties = queueFamilies[i];

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
			if (!graphicsQueueIndex.has_value() && presentSupport)
			{
				presentQueueIndex = i;
			}
		}

		if (!graphicsQueueIndex.has_value() || !presentQueueIndex.has_value())
			throw err::err("one of the queues were not found");
		return std::make_tuple(graphicsQueueIndex.value(), presentQueueIndex.value());
	}

	static auto createLogicalDevices(VkPhysicalDevice physicalDevice, std::tuple<std::uint32_t, std::uintptr_t> queueIndexes)
	{
		// Make queue create information

		VkDeviceQueueCreateInfo queueCreateInfo[2] = {};
		float queuePriority = 1.0f;

		queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[0].queueFamilyIndex = std::get<0>(queueIndexes);
		queueCreateInfo[0].queueCount = 1;
		queueCreateInfo[0].pQueuePriorities = &queuePriority;

		queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[0].queueFamilyIndex = static_cast<std::uint32_t>(std::get<1>(queueIndexes));
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

		// Some device features we can toggle
		// Note: This version is ugly, I'll like to go for a compile-time configurator to be applied during run-time


		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.shaderClipDistance = VK_TRUE;
		enabledFeatures.shaderCullDistance = VK_TRUE;
		enabledFeatures.samplerAnisotropy  = VK_TRUE;

		const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		if constexpr (utils::vulkanDbg) {
			deviceCreateInfo.enabledLayerCount = static_cast<std::uint32_t>(utils::vulkanDebugLayerName.size());
			deviceCreateInfo.ppEnabledLayerNames = utils::vulkanDebugLayerName.data();
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

	static auto getQueues(VkDevice device, std::tuple<std::uint32_t, std::uintptr_t> queueIndexes)
	{
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		vkGetDeviceQueue(device, std::get<0>(queueIndexes), 0, &graphicsQueue);
		vkGetDeviceQueue(device, std::get<1>(queueIndexes), 0, &presentQueue);

		return std::make_tuple(graphicsQueue, presentQueue);
	}

	[[maybe_unused]]
	static auto passDebuggerFunc(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callbackFunc)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = callbackFunc;
		createInfo.pUserData = nullptr;

		auto CreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		VkDebugUtilsMessengerEXT callback;

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

		std::vector<types::Vertex> vertices = {
			{ { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
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
		} stageBuffers;


		// We create our command buffer

		VkCommandBufferAllocateInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufInfo.commandPool = commandPool;
		cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufInfo.commandBufferCount = 1;

		VkCommandBuffer copyCommandBuffer;
		vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer);

		// Buffer handling

		const auto vertexBuffer(new types::VertexBuffer);

		void* tempData;

		// Handle vertices stage buffer

		std::function<void()> createVertices = [&] {
			// Create buffer on the CPU

			VkBufferCreateInfo vertexBufferInfo = {};
			vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferInfo.size = verticesSize;
			vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stageBuffers.vertices.buffer);

			// Set up buffer to host

			vkGetBufferMemoryRequirements(device, stageBuffers.vertices.buffer, &memoryRequirements);
			memoryAllocation.allocationSize = memoryRequirements.size;
			getMemoryType(deviceMemoryProps, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memoryAllocation.memoryTypeIndex);
			vkAllocateMemory(device, &memoryAllocation, nullptr, &stageBuffers.vertices.memory);

			vkMapMemory(device, stageBuffers.vertices.memory, 0, verticesSize, 0, &tempData);
			memcpy(tempData, vertices.data(), verticesSize);
			vkUnmapMemory(device, stageBuffers.vertices.memory);
			vkBindBufferMemory(device, stageBuffers.vertices.buffer, stageBuffers.vertices.memory, 0);

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

			VkBufferCreateInfo indexBufferInfo = {};
			indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			indexBufferInfo.size = indicesSize;
			indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			vkCreateBuffer(device, &indexBufferInfo, nullptr, &stageBuffers.indices.buffer);
			vkGetBufferMemoryRequirements(device, stageBuffers.indices.buffer, &memoryRequirements);
			memoryAllocation.allocationSize = memoryRequirements.size;
			getMemoryType(deviceMemoryProps, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memoryAllocation.memoryTypeIndex);
			vkAllocateMemory(device, &memoryAllocation, nullptr, &stageBuffers.indices.memory);
			vkMapMemory(device, stageBuffers.indices.memory, 0, indicesSize, 0, &tempData);
			memcpy(tempData, indices.data(), indicesSize);
			vkUnmapMemory(device, stageBuffers.indices.memory);
			vkBindBufferMemory(device, stageBuffers.indices.buffer, stageBuffers.indices.memory, 0);

			// And allocate another gpu only buffer for indices
			indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			vkCreateBuffer(device, &indexBufferInfo, nullptr, &vertexBuffer->index);
			vkGetBufferMemoryRequirements(device, vertexBuffer->index, &memoryRequirements);
			memoryAllocation.allocationSize = memoryRequirements.size;
			getMemoryType(deviceMemoryProps, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocation.memoryTypeIndex);
			vkAllocateMemory(device, &memoryAllocation, nullptr, &vertexBuffer->indexMemory);
			vkBindBufferMemory(device, vertexBuffer->index, vertexBuffer->indexMemory, 0);

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(copyCommandBuffer, &bufferBeginInfo);

			VkBufferCopy copyRegion = {};
			copyRegion.size = verticesSize;
			vkCmdCopyBuffer(copyCommandBuffer, stageBuffers.vertices.buffer, vertexBuffer->buffer, 1, &copyRegion);
			copyRegion.size = indicesSize;
			vkCmdCopyBuffer(copyCommandBuffer, stageBuffers.indices.buffer, vertexBuffer->index, 1, &copyRegion);

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

		vkDestroyBuffer(device, stageBuffers.vertices.buffer, nullptr);
		vkFreeMemory(device, stageBuffers.vertices.memory, nullptr);
		vkDestroyBuffer(device, stageBuffers.indices.buffer, nullptr);
		vkFreeMemory(device, stageBuffers.indices.memory, nullptr);

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

		const auto uniformBuffer(new types::UniformBuffer);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(uniformBuffer->uniformData.transformationMatrix);
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

		auto swapchainInformation(new types::SwapchainInformation);

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

		swapchainInformation->extent = chooseSwapExtent(surfaceCapabilities);

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
		createInfo.imageExtent = swapchainInformation->extent;
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

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchainInformation->swapchain) != VK_SUCCESS)
			throw err::err("failed to create swapchain!");

		if (oldSwapchain != nullptr)
			vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

		oldSwapchain = swapchainInformation->swapchain;
		swapchainInformation->format = surfaceFormat.format;

		auto actualImageCount = 0u;
		if (vkGetSwapchainImagesKHR(device, swapchainInformation->swapchain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0)
			throw err::err("failed to get number of swapchain images");

		swapchainInformation->images.resize(actualImageCount);

		if (vkGetSwapchainImagesKHR(device, swapchainInformation->swapchain, &actualImageCount, swapchainInformation->images.data()) != VK_SUCCESS)
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

	static auto createImageViews(VkDevice device, VkFormat swapChainFormat, std::vector<VkImage>& swapChainImages)
	{
		// Resize to account for these images
		const auto imageCount{ swapChainImages.size() };

		std::vector<VkImageView> swapChainImageViews;
		swapChainImageViews.resize(imageCount);

		// Create one image view for every swapchain image

		for (auto i = 0u; i < imageCount; i++)
			swapChainImageViews[i] = createImageView(device, swapChainImages[i], swapChainFormat);
		
		return swapChainImageViews;
	}

	static auto createFrameBuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView> swapChainImageViews, types::SwapchainInformation* swapchainInformation)
	{
		std::vector<VkFramebuffer> swapChainFramebuffers;
		swapChainFramebuffers.resize(swapchainInformation->images.size());

		for (auto i = 0u; i < swapchainInformation->images.size(); i++) {
			VkFramebufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = renderPass;
			createInfo.attachmentCount = 1;
			createInfo.pAttachments = &swapChainImageViews[i];
			createInfo.width = swapchainInformation->extent.width;
			createInfo.height = swapchainInformation->extent.height;
			createInfo.layers = 1;

			if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw err::err("failed to create a frame buffer at {}", i);
			}
		}

		return swapChainFramebuffers;
	}

	static auto createGraphicsPass(VkDevice device, VkFormat swapChainFormat)
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

		// Note: hardware will automatically transition attachment to the specified layout
		// Note: index refers to attachment descriptions array
		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Note: this is a description of how the attachments of the render pass will be used in this sub pass
		// e.g. if they will be read in shaders and/or drawn to
		VkSubpassDescription subPassDescription = {};
		subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPassDescription.colorAttachmentCount = 1;
		subPassDescription.pColorAttachments = &colorAttachmentReference;

		// Create the render pass
		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachmentDescription;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDescription;

		VkRenderPass renderPass;

		const auto res = vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);
		if (res != VK_SUCCESS)
			throw err::err("failed to create a render pass");
		return renderPass;
	}

	static auto createRenderingPipeline(VkDevice device, types::VertexInputBindingDescriptors* Vertexdescriptors, VkRenderPass renderPass, VkExtent2D swapchainExtent)
	{
		// Pipeline information

		auto pipelineInfo(new types::graphicsPipelineInformation);

		// Make up shader stages

		const auto shaderModules = createShaderModules(device);

		VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
		vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderCreateInfo.module = std::get<0>(shaderModules);
		vertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
		fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderCreateInfo.module = std::get<1>(shaderModules);
		fragmentShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &Vertexdescriptors->main;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
		vertexInputCreateInfo.pVertexAttributeDescriptions = Vertexdescriptors->attributes.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;


		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = swapchainExtent.width;
		scissor.extent.height = swapchainExtent.height;


		VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
		rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationCreateInfo.depthBiasClamp = 0.0f;
		rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendCreateInfo.blendConstants[0] = 0.0f;
		colorBlendCreateInfo.blendConstants[1] = 0.0f;
		colorBlendCreateInfo.blendConstants[2] = 0.0f;
		colorBlendCreateInfo.blendConstants[3] = 0.0f;

		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
		descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCreateInfo.bindingCount = 1;
		descriptorLayoutCreateInfo.pBindings = &layoutBinding;

		// Now finilize some stuff


		if (vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &pipelineInfo->descriptor) != VK_SUCCESS)
			throw err::err("failed to create a descriptor layout");

		VkPipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.setLayoutCount = 1;
		layoutCreateInfo.pSetLayouts = &pipelineInfo->descriptor;

		if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineInfo->layout) != VK_SUCCESS)
			throw err::err("failed to create a pipeline layout");

		// Create the graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
		pipelineCreateInfo.layout = pipelineInfo->layout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		const auto res = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &pipelineInfo->pipeline);
		if (res != VK_SUCCESS)
			throw err::err("failed to create graphics pipeline");

		vkDestroyShaderModule(device, std::get<0>(shaderModules), nullptr); // Destroy vertex shader
		vkDestroyShaderModule(device, std::get<1>(shaderModules), nullptr); // Destroy fragment shader

		return pipelineInfo;
	}

	static auto createCommandBuffers(VkDevice device, VkCommandPool commandPool, VkRenderPass renderPass, types::SwapchainInformation* swapchainInfo,
		std::tuple<std::uint32_t, std::uint32_t> queueIndexes, std::tuple<types::VertexBuffer* , types::VertexInputBindingDescriptors*> vertexBuffInfo, 
		std::vector<VkFramebuffer> swapChainFrameBuffers, types::graphicsPipelineInformation* pipeLineInfo, VkDescriptorSet descriptorSet)
	{
		std::vector<VkCommandBuffer> graphicsCommandBuffers;
		graphicsCommandBuffers.resize(swapchainInfo->images.size());

		// Allocate a command buffer

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<std::uint32_t>(swapchainInfo->images.size());

		if (vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS)
			throw err::err("can't allocate a command buffer, do you have enough memory?");

		// Handle Some descriptors for the screening

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkImageSubresourceRange subResourceRange = {};
		subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.levelCount = 1;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.layerCount = 1;

		VkClearValue clearColor = {
			{ 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
		};

		// Make a buffer for every swapchain image
		for (auto i = 0u; i < swapchainInfo->images.size(); i++)
		{
			vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);

			// If present queue family and graphics queue family are different, then a barrier is necessary
			// The barrier is also needed initially to transition the image to the present layout

			VkImageMemoryBarrier presentToDrawBarrier = {};
			presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			presentToDrawBarrier.srcAccessMask = 0;
			presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			if (std::get<0>(queueIndexes) != std::get<1>(queueIndexes)) {
				presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}
			else {
				presentToDrawBarrier.dstQueueFamilyIndex = std::get<0>(queueIndexes);
				presentToDrawBarrier.srcQueueFamilyIndex = std::get<1>(queueIndexes);
			}

			presentToDrawBarrier.image = swapchainInfo->images[i];
			presentToDrawBarrier.subresourceRange = subResourceRange;

			vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = swapChainFrameBuffers[i];
			renderPassBeginInfo.renderArea.offset.x;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent = swapchainInfo->extent;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindDescriptorSets(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLineInfo->layout, 0, 1, &descriptorSet, 0, nullptr);

			vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLineInfo->pipeline);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(graphicsCommandBuffers[i], 0, 1, &std::get<0>(vertexBuffInfo)->buffer, &offset);

			vkCmdBindIndexBuffer(graphicsCommandBuffers[i], std::get<0>(vertexBuffInfo)->index, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(graphicsCommandBuffers[i], 3, 1, 0, 0, 0);

			vkCmdEndRenderPass(graphicsCommandBuffers[i]);

			// If present and graphics queue families differ, then another barrier is required
 
			if (std::get<0>(queueIndexes) != std::get<1>(queueIndexes)) {
				VkImageMemoryBarrier drawToPresentBarrier = {};
				drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				drawToPresentBarrier.dstQueueFamilyIndex = std::get<0>(queueIndexes);;
				drawToPresentBarrier.srcQueueFamilyIndex = std::get<1>(queueIndexes);
				drawToPresentBarrier.image = swapchainInfo->images[i];
				drawToPresentBarrier.subresourceRange = subResourceRange;

				vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &drawToPresentBarrier);
			}

			if (vkEndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS)
				throw err::err("couldn't record command buffer, at {}", i);
		}

		vkDestroyPipelineLayout(device, pipeLineInfo->layout, nullptr);

		return graphicsCommandBuffers;
	}
}