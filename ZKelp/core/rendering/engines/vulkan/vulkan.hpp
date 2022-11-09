/*
*	Desc: Vulkan core
*	Note: This is all ugly boiler-plate code for vulkan
*/
#pragma once
#include <cstdint>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../../../../utilities/utilFlags.hpp"
#include "../../../../utilities/console/err.hpp"

#include "builders/builders.hpp"

namespace vulkan
{
	static bool windowResized{ false };

	namespace funcs
	{
		static auto __stdcall debugLayer(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) {

			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				throw err::err("vulkan hidden layer: [{}]", pCallbackData->pMessage);
			return 0u;
		}
	}

	class VulkanEngine
	{
		VkPhysicalDevice physicalDevice;
		VkCommandPool commandPool;
		VkRenderPass renderPass;
		VkRenderPass graphicsPass;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;

		types::UniformBuffer* uniformBuffer;
		types::SwapchainInformation* swapchainInfo;
		types::graphicsPipelineInformation* graphicsPipelineInfo;

		VkDebugReportCallbackEXT callback;
		VkSwapchainKHR oldSwapChain{ nullptr };

		std::vector<VkCommandBuffer> graphicsCommandBuffers;
		std::vector<VkImageView> imageViews;
		std::vector<VkFramebuffer> frameBuffers;

		std::vector<types::Texture> textures;

		std::tuple<VkDevice, VkPhysicalDeviceMemoryProperties> logicalDevices;
		std::tuple<std::uint32_t, std::uint32_t> queueIndexes;
		std::tuple<VkInstance, VkSurfaceKHR> instanceAndSurface;
		std::tuple<VkQueue, VkQueue> queues;
		std::tuple<VkSemaphore, VkSemaphore> semaphores;
		std::tuple<types::VertexBuffer*, types::VertexInputBindingDescriptors*> vertexBufferInfo;

	public:
		void setup(GLFWwindow* window)
		{
			// Setup first instances at vulkan

			glfwSetWindowSizeCallback(window, VulkanEngine::onWindowResized);

			instanceAndSurface = createInstanceAndSurface(window);
			
			if constexpr (utils::vulkanDbg)
				passDebuggerFunc(std::get<0>(instanceAndSurface), funcs::debugLayer);

			// Setup rest

			physicalDevice = findPhysicalDevice(std::get<0>(instanceAndSurface));
			queueIndexes = getQueueIndexes(physicalDevice, std::get<1>(instanceAndSurface));
			logicalDevices = createLogicalDevices(physicalDevice, queueIndexes);
			queues = getQueues(std::get<0>(logicalDevices), queueIndexes);
			semaphores = createSemaphores(std::get<0>(logicalDevices));
			commandPool = createCommandPool(std::get<0>(logicalDevices), std::get<0>(queueIndexes));
			vertexBufferInfo = createVertexBuffer(logicalDevices, std::get<0>(queues), commandPool);
			uniformBuffer = createUniformBuffer(logicalDevices);
			swapchainInfo = createSwapChain(std::get<0>(logicalDevices), physicalDevice, std::get<1>(instanceAndSurface), oldSwapChain);
			renderPass = makeRenderPass(std::get<0>(logicalDevices), swapchainInfo->format);
			imageViews = createImageViews(std::get<0>(logicalDevices), swapchainInfo->format, swapchainInfo->images);
			frameBuffers = createFrameBuffers(std::get<0>(logicalDevices), renderPass, imageViews, swapchainInfo);
			graphicsPass = createGraphicsPass(std::get<0>(logicalDevices), swapchainInfo->format);
			graphicsPipelineInfo = createRenderingPipeline(std::get<0>(logicalDevices), std::get<1>(vertexBufferInfo), renderPass, swapchainInfo->extent);
			descriptorPool = createDescriptorPool(std::get<0>(logicalDevices));
			descriptorSet = createDescriptorSet(std::get<0>(logicalDevices), descriptorPool, graphicsPipelineInfo->descriptor, uniformBuffer);

			graphicsCommandBuffers = createCommandBuffers(std::get<0>(logicalDevices), commandPool, renderPass, swapchainInfo, queueIndexes, vertexBufferInfo, frameBuffers, graphicsPipelineInfo, descriptorSet);
		}

		void cleanup(bool fullclean)
		{
			vkDeviceWaitIdle(std::get<0>(logicalDevices));

			vkFreeCommandBuffers(std::get<0>(logicalDevices), commandPool, graphicsCommandBuffers.size(), graphicsCommandBuffers.data());

			vkDestroyPipeline(std::get<0>(logicalDevices), graphicsPipelineInfo->pipeline, nullptr);
			vkDestroyRenderPass(std::get<0>(logicalDevices), renderPass, nullptr);

			for (auto i = 0u; i < swapchainInfo->images.size(); i++) {
				vkDestroyFramebuffer(std::get<0>(logicalDevices), frameBuffers[i], nullptr);
				vkDestroyImageView(std::get<0>(logicalDevices), imageViews[i], nullptr);
			}

			vkDestroyDescriptorSetLayout(std::get<0>(logicalDevices), graphicsPipelineInfo->descriptor, nullptr);

			if (fullclean) {

				vkDestroySemaphore(std::get<0>(logicalDevices), std::get<0>(semaphores), nullptr);
				vkDestroySemaphore(std::get<0>(logicalDevices), std::get<1>(semaphores), nullptr);

				vkDestroyCommandPool(std::get<0>(logicalDevices), commandPool, nullptr);

				// Clean up uniform buffer related objects

				vkDestroyDescriptorPool(std::get<0>(logicalDevices), descriptorPool, nullptr);
				vkDestroyBuffer(std::get<0>(logicalDevices), uniformBuffer->buffer, nullptr);
				vkFreeMemory(std::get<0>(logicalDevices), uniformBuffer->memory, nullptr);

				// Buffers must be destroyed after no command buffers are referring to them anymore

				vkDestroyBuffer(std::get<0>(logicalDevices), std::get<0>(vertexBufferInfo)->buffer, nullptr);
				vkFreeMemory(std::get<0>(logicalDevices), std::get<0>(vertexBufferInfo)->memory, nullptr);
				vkDestroyBuffer(std::get<0>(logicalDevices), std::get<0>(vertexBufferInfo)->index, nullptr);
				vkFreeMemory(std::get<0>(logicalDevices), std::get<0>(vertexBufferInfo)->indexMemory, nullptr);

				// Note: implicitly destroys images (in fact, we're not allowed to do that explicitly)

				vkDestroySwapchainKHR(std::get<0>(logicalDevices), swapchainInfo->swapchain, nullptr);

				for (auto i = 0u; i < textures.size(); i++)
				{
					const auto& texture = textures[i];

					vkDestroySampler(std::get<0>(logicalDevices), texture.sampler, nullptr);
					vkDestroyImageView(std::get<0>(logicalDevices), texture.view, nullptr);
					vkDestroyImage(std::get<0>(logicalDevices), texture.image, nullptr);
					vkFreeMemory(std::get<0>(logicalDevices), texture.memory, nullptr);
				}

				vkDestroyDevice(std::get<0>(logicalDevices), nullptr);

				vkDestroySurfaceKHR(std::get<0>(instanceAndSurface), std::get<1>(instanceAndSurface), nullptr);

				if constexpr(utils::vulkanDbg) {
					PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(std::get<0>(instanceAndSurface), "vkDestroyDebugReportCallbackEXT");
					DestroyDebugReportCallback(std::get<0>(instanceAndSurface), callback, nullptr);
				}

				vkDestroyInstance(std::get<0>(instanceAndSurface), nullptr);
			}
		}

		void resetView()
		{
			windowResized = false;
			cleanup(false);

			swapchainInfo = createSwapChain(std::get<0>(logicalDevices), physicalDevice, std::get<1>(instanceAndSurface), oldSwapChain);
			renderPass = makeRenderPass(std::get<0>(logicalDevices), swapchainInfo->format);
			imageViews = createImageViews(std::get<0>(logicalDevices), swapchainInfo->format, swapchainInfo->images);
			frameBuffers = createFrameBuffers(std::get<0>(logicalDevices), renderPass, imageViews, swapchainInfo);
			graphicsPipelineInfo = createRenderingPipeline(std::get<0>(logicalDevices), std::get<1>(vertexBufferInfo), renderPass, swapchainInfo->extent);
			graphicsCommandBuffers = createCommandBuffers(std::get<0>(logicalDevices), commandPool, renderPass, swapchainInfo, queueIndexes, vertexBufferInfo, frameBuffers, graphicsPipelineInfo, descriptorSet);
		}

		void passPresentQueue(std::uint32_t imageIndex)
		{
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &std::get<1>(semaphores);

			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchainInfo->swapchain;
			presentInfo.pImageIndices = &imageIndex;

			const auto res = vkQueuePresentKHR(std::get<1>(queues), &presentInfo);

			if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || windowResized)
				resetView();
			else if(res != VK_SUCCESS)
				throw err::err("something failed while passing present queue");
		}

		void submitImage(std::uint32_t imageIndex)
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &std::get<0>(semaphores);

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &std::get<1>(semaphores);

			// This is the stage where the queue should wait on the semaphore
			VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			submitInfo.pWaitDstStageMask = &waitDstStageMask;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex];

			if (vkQueueSubmit(std::get<0>(queues), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
				throw err::err("could not submit command buffer");
		}

		std::optional<std::uint32_t> acquireImage()
		{
			auto imageIndex{ 0u };

			// Device is dangling, so its null. Causes segfault.

			VkResult res = vkAcquireNextImageKHR(std::get<0>(logicalDevices), swapchainInfo->swapchain, UINT64_MAX, std::get<0>(semaphores), VK_NULL_HANDLE, &imageIndex);

			if (res == VK_ERROR_OUT_OF_DATE_KHR)
			{
				resetView();
				return {};
			}
			else if(res != VK_SUCCESS)
			{
				throw err::err("couldn't aquire image");
			}
			return std::make_optional(imageIndex);
		}

		static void onWindowResized(GLFWwindow* window, int width, int height) {
			windowResized = true;
		}

		////// Utilities

		// Make functions

		void makeTexture(const std::string_view& path)
		{
			// Make texture from our existing images

			textures.push_back(createTexture(std::get<0>(logicalDevices), physicalDevice, std::get<0>(queues), commandPool, path));
		}

	} vulkanEngine;
}