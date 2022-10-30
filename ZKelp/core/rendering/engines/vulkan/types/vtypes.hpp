/*	
*	Desc: Vulkan types
*	Note: These types are specific to the vulkan graphics API
*/
#pragma once
#include "../../../../types/common.hpp"

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

		types::Vec<VkImage> images;
	};
}