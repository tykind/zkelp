/*
*	Desc: Dynamic Scheduler
*	Note: Security feateres still underminded with this
*/
#pragma once
#include <iostream>
#include <thread>
#include <string_view>
#include <functional>
#include <memory>
#include <cstdint>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define SCHEDULER_TYPEID const std::type_info &type = typeid(*this)
namespace zkelp 
{
	namespace scheduler_types 
	{
		struct BaseTask 
		{
			std::string_view task_name{ "Unamed" };
			std::optional<std::chrono::system_clock::duration> start_time;

			virtual bool mainTick() { return false; };
		};

		struct concurrentTask final : BaseTask
		{
			std::function<void()> to_run;

			SCHEDULER_TYPEID;

			virtual bool mainTick() {
				std::printf("%s -> run time : %f seconds\n", task_name.data(), start_time.has_value()  ? static_cast<std::float_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - start_time.value()).count()) / 1000.0f : 0);
				to_run();
				return true;
			}
		};  
	}

	class dynamic_scheduler_t
	{
		std::vector<scheduler_types::BaseTask*> tasks;
		std::optional<std::jthread> singleton;
		GLFWwindow* renderingWindow{ nullptr };

		std::uint32_t run_timer{ 0u };
	public:

		void change_tick(const std::uint32_t& new_timer)
		{
			run_timer = new_timer;
		}

		void add_task(zkelp::scheduler_types::BaseTask* task) {
			tasks.push_back(task);
		};

		auto getRenderingWindow()
		{
			return renderingWindow;
		}

		bool run() {
			if (!singleton.has_value()) {
				// Make and set a singleton-running thread
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
				singleton = std::make_optional(std::jthread([&] {

					renderingWindow = glfwCreateWindow(800, 600, utils::applicationName, nullptr, nullptr);

					while (true) {
						std::this_thread::sleep_for(std::chrono::milliseconds(run_timer)); // Prevent a crash

						// Loop through the tasks with their piorities listed
						for (auto& task : tasks) {
							if (task->mainTick())
								if (!task->start_time.has_value())
									task->start_time = std::make_optional(std::chrono::system_clock::now().time_since_epoch()); // Give time if the time isn't provided
						}
					}
					}));
			}
			return true;
		};
	} dynamic_scheduler;

}