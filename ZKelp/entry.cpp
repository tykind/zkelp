/*
*	Desc: ZKelp Engine
*	Note: Finish on rendering engine / Using Vulkan / Handle windows resize montitor event
* 
*	TODO: Finish functions to prepare texture image
* 
*	Author: Tykind
*/

#include "utilities/syntax-sugar/cify.hpp"
#include "utilities/console/err.hpp"
#include "utilities/console/logger.hpp"

#include "core/scheduler/scheduler.hpp"
#include "core/rendering/rendering.hpp"

int main() {
	SetConsoleTitleA(utils::applicationName);
	logger.log("Welcome to Zkelp\n");

	try
	{
		logger.log("Starting up renderer...\n");
		const auto renderingTask = zkelp::initRenderer();

		//std::atexit(gnr::cify([renderingTask]() { renderingTask->cleanScene(); }));

		zkelp::dynamic_scheduler.run();
	} catch (err::err& err) {
		logger.log(err.what().c_str());
		std::terminate();
	}
}