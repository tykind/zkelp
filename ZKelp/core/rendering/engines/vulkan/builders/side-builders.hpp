/*
*	Desc: Side functions which are used within the main builder
*	Note: This isn't used in the main engine, it's just on the builder
*/
#include <cstdint>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vulkan
{
	static bool getMemoryType(VkPhysicalDeviceMemoryProperties deviceMemoryProps, uint32_t typeBits, VkFlags props, uint32_t* typeIndex)
	{
		auto ret{ false };
		 
		for (auto i = 0u; i < 32; i++)
		{
			if ((typeBits & 1) == 1)
				if ((deviceMemoryProps.memoryTypes[i].propertyFlags & props) == props) {
					*typeIndex = i;
					ret = true;
					break;
				}
			typeBits >>= 1;
		}

		return ret;
	}

	static auto choosePresentMode(const std::vector<VkPresentModeKHR> presentModes)
	{
		VkPresentModeKHR choosenPresentMode{ VK_PRESENT_MODE_FIFO_KHR };

		for (const auto& presentMode : presentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				choosenPresentMode = presentMode;
				break;
			}
		}

		return choosenPresentMode;
	}

	static auto chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		if (surfaceCapabilities.currentExtent.width == -1)
		{
			VkExtent2D swapChainExtent;

			swapChainExtent.width = std::min(std::max(utils::windowInformation[0], surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
			swapChainExtent.height = std::min(std::max(utils::windowInformation[1], surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

			return swapChainExtent;
		}

		return surfaceCapabilities.currentExtent;
	}

	static auto chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		std::optional<VkSurfaceFormatKHR> ret{ std::nullopt };
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			ret = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		}

		if (!ret.has_value())
		{
			for (const auto& availableSurfaceFormat : availableFormats)
				if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
					ret = availableSurfaceFormat;
					break;
				}
		}

		return ret.has_value() ? ret.value() : availableFormats[0];
	}
}