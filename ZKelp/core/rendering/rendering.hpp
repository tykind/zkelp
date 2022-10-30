/*
*	Desc: Bases of a rendering engine
*	Note: This is still under design choises, so this whole thing might be rewritten differently
*/
#pragma once
#include <Windows.h>
#include <chrono>

#include <GLFW/glfw3.h>

#include "../../utilities/utilFlags.hpp"
#include "../../utilities/console/err.hpp"

#include "../scheduler/scheduler.hpp"

namespace zkelp
{
	namespace scheduler_types
	{
		struct RenderingTask final : public BaseTask
		{
			std::string_view taskName{ "rendering task" };
			std::function<void(RenderingTask*)> toRun{ nullptr };

			GLFWwindow* window{ nullptr };

			void cleanScene()
			{
				glfwDestroyWindow(window);
				glfwTerminate();
			}

			virtual bool mainTick() {
				// Run a frame
				toRun(this);
				return true;
			}
		};
	}

	static scheduler_types::RenderingTask* initRenderer()
	{
		if (!glfwInit())
			throw err::err("renderer failed to initialized");

		auto mainRenderTask(new scheduler_types::RenderingTask());
		mainRenderTask->window = glfwCreateWindow(800, 600, utils::applicationName, nullptr, nullptr);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		
		glfwMakeContextCurrent(mainRenderTask->window);
		glfwSwapInterval(1);

		if constexpr (utils::useVulkan)
		{
			if (!glfwVulkanSupported())
				throw err::err("vulkan not supported");

			// Create vulkan device

		}

		mainRenderTask->toRun = [&](scheduler_types::RenderingTask* task) {
			if (!glfwWindowShouldClose(task->window)) {
				glfwPollEvents();

				// Beging drawing

				glfwSwapBuffers(task->window);
			}
			else
			{
				task->cleanScene();
				std::terminate();
			}
		};

		zkelp::dynamic_scheduler.add_task(mainRenderTask);
		return mainRenderTask;
	}
}