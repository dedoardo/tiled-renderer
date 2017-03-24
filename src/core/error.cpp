/* error.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/error.hpp>

// Windows
#include <Windows.h>

namespace camy
{
	namespace hidden
	{
		std::ostream* g_output_stream{ &std::cout };
	}

	std::ostream* get_output_stream()
	{
		return hidden::g_output_stream;
	}

	void set_output_stream(std::ostream* output_stream)
	{
		hidden::g_output_stream = output_stream;

	}

	CONSOLE_SCREEN_BUFFER_INFO last_sbi;

	namespace hidden
	{
		void set_info_color()
		{
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &last_sbi);
			HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(cout_handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}

		void set_warning_color()
		{
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &last_sbi);
			HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(cout_handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}

		void set_error_color()
		{
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &last_sbi);
			HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(cout_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		}

		void restore_color()
		{
			HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(cout_handle, last_sbi.wAttributes);
		}

		void debug_break()
		{
			DebugBreak();
		}
	}
}