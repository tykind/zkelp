/*	
*	Desc: Vulkan types
*	Note: These types are specific to the vulkan graphics API
*/
#pragma once
#include "../../../../types/common.hpp"
#include "../../../../../3rd party/glm/glm.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace types
{
	struct VertexInputBindingDescriptors
	{
		VkVertexInputBindingDescription main;
		std::vector<VkVertexInputAttributeDescription> attributes;
	};

	struct SwapchainInformation
	{
		VkSwapchainKHR swapchain;
		VkFormat format;
		VkExtent2D extent;

		std::vector<VkImage> images;
	};

	struct graphicsPipelineInformation
	{
		VkPipeline pipeline;
		VkPipelineLayout layout;
		VkDescriptorSetLayout descriptor;
		VkVertexInputBindingDescription vertexBindingDescription;
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
	};

	struct VertexBuffer
	{
		VkBuffer buffer;
		VkDeviceMemory memory;

		VkBuffer index;
		VkDeviceMemory indexMemory;
	};

	struct UniformBuffer
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
		
		struct
		{
			Matrix<double> transformationMatrix;
		} uniformData;
	};

	struct Texture
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkSampler sampler;
	};

	struct Vertex
	{
		float pos[3];
		float color[3];
	};
}