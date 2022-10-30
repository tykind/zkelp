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

namespace vulkan
{
	namespace funcs
	{
		static bool debugLayer(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t dbgCode, const char* layerName, const char* dbgMessage, void* userData)
		{
			if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
				throw err::err("vulkan err [{}]: {} ({})", layerName, dbgMessage, dbgCode);
			else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
				throw err::err("vulkan err [{}]: {} ({})", layerName, dbgMessage, dbgCode);
			return false;
		}
	}

	struct VulkanEngine
	{
		void setup()
		{
			// Set up vulkan stuff
			
		}
	} vulkanEngine;
}