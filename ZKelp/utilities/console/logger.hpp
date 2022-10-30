/*
*	Desc: Logger-based with context... handles pretty well
*	Note: This isn't special, but it's neat. Feel free to use in your projects!
*/
#pragma once
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <chrono>


#include "../utilFlags.hpp"

#define LOG_CONTEXT(timeStr, context) std::printf("[\x1B[90m\x1B[2m%s\x1B[22m\x1B[39m] [\x1B[92m\x1B[1m%s\x1B[22m\x1B[39m] - ", timeStr, context)
#define GET_HEX_FORMAT(x) std::format("\x1B[33m\x1B[1m{:X}\x1B[22m\x1B[39m", x).c_str()

class Logger
{
	std::string_view contextName{ utils::mainContextName };

	auto getTime()
	{
		auto now = std::chrono::system_clock::now();
		auto start_of_day = std::chrono::floor<std::chrono::days>(now);
		auto time_from_start_day = std::chrono::round<std::chrono::seconds>(now - start_of_day);

		std::chrono::hh_mm_ss hms{ time_from_start_day };

		return std::format("{}", hms);
	}
public:
	std::stringstream publicStream;

	// Misc functions

	template<typename _Ty>
	void changeContextName(_Ty input)
	{
		if constexpr (std::is_same<_Ty, typename std::string::value_type>::value)
			contextName = input.c_str();
		else
			contextName = input;
	}

	// Log functions

	void logArray(std::uint8_t* data, std::uint32_t sz, bool raw = false)
	{
		for (auto i = 0u; i < sz; i++)
		{
			if (!raw)
				std::cout << std::hex << static_cast<int>(data[i]) << " ";
			else
				std::cout << data[i];
		}
		std::cout << std::endl;
	}

	void logSream(bool raw = false, bool dont_clear = false)
	{
		std::uint8_t c;
		while (publicStream >> c)
		{
			if (!raw)
				std::cout << std::hex << static_cast<int>(c) << " ";
			else
				std::cout << c;
		}
		if (!dont_clear)
			publicStream.clear();
		std::cout << std::endl;
	}

	template<typename ..._Args>
	void log(std::string_view str, _Args ...fmt)
	{
		LOG_CONTEXT(getTime().c_str(), contextName.data());
		std::printf(str.data(), std::forward<_Args>(fmt)...);
	}
} logger;

#undef LOG_CONTEXT