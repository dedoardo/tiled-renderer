/* error.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/utils.hpp>

// C++ STL
#include <iostream>
#include <initializer_list>
#include <string>
#include <cassert>
#include <chrono>
#include <iomanip>

namespace camy
{
	namespace hidden
	{
		extern std::ostream* g_output_stream;
	}

	void set_output_stream(std::ostream* output_stream);
	std::ostream* get_output_stream();

	namespace hidden
	{	
		void set_info_color();
		void set_warning_color();
		void set_error_color();
		void restore_color();

		/*
			Note: I do admit that this method of logging might look kinda disgusting
			considering the way cleaner std::initializer_list approach, but this
			way arguments do not need to be converted to strings when passed to the
			log function, resulting in extremely cleaner logging code. In addition
			no code is generated when logging is disabled
		*/
		template <typename OutArg>
		void log_rec(const OutArg* arg)
		{
			if (arg == nullptr)
				*g_output_stream << "<none>";
			else
				*g_output_stream << arg;
		}

		template<typename OutArg>
		void log_rec(const OutArg& arg)
		{	
			*g_output_stream << arg;
		}

		template <typename FirstOutArg, typename SecondOutArg, typename ...OutArgs>
		void log_rec(const FirstOutArg* first_out_arg, const SecondOutArg& second_out_arg, const OutArgs& ...out_args)
		{
			if (first_out_arg == nullptr)
				*g_output_stream << "<none>";
			else
				*g_output_stream << first_out_arg;
			log_rec(second_out_arg, out_args...);
		}

		template <typename FirstOutArg, typename SecondOutArg, typename ...OutArgs>
		void log_rec(const FirstOutArg& first_out_arg, const SecondOutArg& second_out_arg, const OutArgs& ...out_args)
		{
			*g_output_stream << first_out_arg;
			log_rec(second_out_arg, out_args...);
		}

#pragma warning(disable:4996)
		template <typename ...OutArgs>
        camy_api void log(const char8* tag, const char8* function, const char8* line, const OutArgs& ...out_args)
		{
			if (g_output_stream == nullptr) return;
	
			if (g_output_stream == &std::cout)
			{
				if (::camy::strcmp(tag, "Info"))
					set_info_color();
				else if (::camy::strcmp(tag, "Warn"))
					set_warning_color();
				else if (::camy::strcmp(tag, "Err "))
					set_error_color();
			}

			auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			*g_output_stream << "[" << std::put_time(std::localtime(&now_t), "%T") << "]";

			*g_output_stream << "[" << tag << "] ";

			log_rec(out_args...);

			*g_output_stream << " [" << function << ":" << line << "]";
			*g_output_stream << std::endl;

			restore_color();
		}

        template <typename ...OutArgs>
        camy_api void log_stripped(const char8* tag, const OutArgs& ...out_args)
        {
            if (g_output_stream == nullptr) return;

            if (g_output_stream == &std::cout)
            {
                if (::camy::strcmp(tag, "Info"))
                    set_info_color();
                else if (::camy::strcmp(tag, "Warn"))
                    set_warning_color();
                else if (::camy::strcmp(tag, "Err "))
                    set_error_color();
            }

            log_rec(out_args...);
            *g_output_stream << std::endl;

            restore_color();
        }
#pragma warning(default:4996)
	
		void debug_break();
	}
}

/*
	Tests if a state being set is malformed
*/
#ifdef camy_enable_states_validation
#define camy_validate_state(x, error_message) { if ((x) == nullptr) camy_warning(error_message); }
#else
#define camy_validate_state(...)
#endif

#ifdef camy_enable_asserts
#define camy_assert(x) { if (!(x)) (camy::hidden::debug_break()); }
#else
#define camy_assert(...)
#endif

#if camy_enable_logging >= 3
#define camy_error(...) ::camy::hidden::log("Err ", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#define camy_error_stripped(...) ::camy::hidden::log_stripped("Error",  __VA_ARGS__)
#else
#define camy_error(...)
#endif

#if camy_enable_logging >= 2
#define camy_warning(...) ::camy::hidden::log("Warn", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#else
#define camy_warning(...)
#endif

#if camy_enable_logging >= 1
#define camy_info(...) ::camy::hidden::log("Info", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#define camy_info_stripped(...) ::camy::hidden::log_stripped("Info",  __VA_ARGS__)
#else
#define camy_info(...)
#define camy_info_stripped(...)
#endif