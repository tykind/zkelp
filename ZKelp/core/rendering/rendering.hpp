/*
*	Desc: Bases of a rendering engine
*	Note: This is still under design choises, so this whole thing might be rewritten differently
*/
#pragma once
#include <Windows.h>
#include <chrono>
#include <mutex>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../../utilities/utilFlags.hpp"
#include "../../utilities/console/err.hpp"

#include "../scheduler/scheduler.hpp"

#include "engines/vulkan/vulkan.hpp"
 
#define UGLY_VULKAN

namespace zkelp
{
	namespace scheduler_types
	{
		struct RenderingTask final : public BaseTask
		{
			std::string_view taskName{ "rendering task" };
			std::once_flag vulkanInit;

			void cleanScene()
			{
				const auto window = dynamic_scheduler.getRenderingWindow();

				if constexpr (utils::useVulkan)
					vulkan::vulkanEngine.cleanup(true);

				glfwSetWindowShouldClose(window, true);
				glfwDestroyWindow(dynamic_scheduler.getRenderingWindow());
				glfwTerminate();
			}

			virtual bool mainTick() 
			{
				// Rendering main

				const auto window = dynamic_scheduler.getRenderingWindow();

				if (!glfwWindowShouldClose(window)) {
					////// Begin drawing here

					// Check for vulkan

					if constexpr (utils::useVulkan)
					{
						// Setup vulkan stuffs

						std::call_once(vulkanInit, [&] {
							const auto window = dynamic_scheduler.getRenderingWindow();

							// Setup the vulkan engine

							vulkan::vulkanEngine.setup(window);
							glfwSetWindowSizeCallback(window, vulkan::VulkanEngine::onWindowResized);
							
							// Do fuckups

							vulkan::vulkanEngine.makeTexture("D:\\Projects\\ZKelp\\x64\\Debug\\resources\\textures\\swag.png");
						});

						// Fetch vulkan stuff

						const auto imageIndex = vulkan::vulkanEngine.acquireImage();
						if (!imageIndex.has_value())
							return true;

						vulkan::vulkanEngine.submitImage(imageIndex.value());
						vulkan::vulkanEngine.passPresentQueue(imageIndex.value());
					}

					// End drawing here

					glfwPollEvents();
				}
				else
				{
					cleanScene();
					return false;
				}
				return true;
			}
		};
	}

	static scheduler_types::RenderingTask* initRenderer()
	{
		if (!glfwInit())
			throw err::err("renderer failed to initialized");

		auto mainRenderTask(new scheduler_types::RenderingTask());

		// Setup glfw window configurations
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// Create new window

		// Check if vulkan is enabled, then just work with vulkan

		if constexpr (utils::useVulkan)
		{
			if (!glfwVulkanSupported())
				throw err::err("vulkan not supported");
		}

		// Setup & push rendering task

		dynamic_scheduler.add_task(mainRenderTask);
		return mainRenderTask;
	}
}

#undef UGLY_VULKAN