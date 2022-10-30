/*
*	Desc: Flags used by the utilities
*	Note: These are mostly at compile time & can't be changed during runtime
*/
#pragma once
#include <cstdint>
#include <array>

namespace utils {

	// Global

	constexpr auto applicationName{ "ZKELP | Version 1" };
	constexpr auto mainContextName{ "ZKELP" };

	static std::array<std::uint32_t, 2> windowInformation = { 640u, 480u }; // (Width, Height)

	// Vulkan specific

	constexpr auto useVulkan{ true };
	constexpr auto vulkanDbg{ true };
	const char* vulkanDebugLayerName = "VK_LAYER_LUNARG_standard_validation";
}