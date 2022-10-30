/*
*	Desc: Roblox console utility
*	Note: This includes a freeconsole hook, this is annoyingly used by the Main roblox client.
*/
#pragma once
#include <Windows.h>
#include <cstdint>
#include <iostream>

#include "logger.hpp"

namespace utils
{
	constexpr auto freeConsoleHook{ false };
	static auto consoleAllocated{ false };

	static auto allocateConsole(const char* console_name)
	{
		if (!consoleAllocated)
		{
			if constexpr (freeConsoleHook)
			{
				static auto _ptr = reinterpret_cast<std::uintptr_t>(&FreeConsole);
				static auto _ptrjmp = _ptr + 6;

				unsigned long old{ 0u };

				VirtualProtect(&_ptr, 0x6, PAGE_EXECUTE_READWRITE, &old);
				*reinterpret_cast<std::uintptr_t**>(_ptr + 0x2) = &_ptrjmp;
				*reinterpret_cast<std::uint8_t*>(_ptr + 0x63) = 0xC3;
				VirtualProtect(&_ptr, 0x6, old, &old);
			}

			AllocConsole();

			FILE* fs;
			freopen_s(&fs, "CONIN$", "r", stdin);
			freopen_s(&fs, "CONOUT$", "w", stdout);
			freopen_s(&fs, "CONOUT$", "w", stderr);

			#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
				HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
				DWORD dwMode = 0;
				GetConsoleMode(hOut, &dwMode);
				dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				SetConsoleMode(hOut, dwMode);
			#endif

			SetConsoleTitleA(console_name);
			consoleAllocated = true;
		}
		else
		{
			::ShowWindow(GetConsoleWindow(), SW_SHOW);
		}
	}

	static auto deallocateConsole()
	{
		::ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
}