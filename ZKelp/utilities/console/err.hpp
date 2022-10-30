/*
*	Desc: Modern error structure
*	Note: To actually use this you need C++20 or later
*/
#pragma once
#include <string>
#include <format>

namespace err
{
	class err
	{
		std::string _message;
	public:

		auto what()
		{
			return _message;
		}

		template<typename ...Args>
		err(std::string_view msg, Args ...args)
		{
			_message = std::vformat(msg, std::make_format_args(args...)); // This requires C++20
		}
	};
}