#pragma once

// camy
#include "config.hpp"
#include "base.hpp"

// C++ STL
#include <iostream>
#include <initializer_list>
#include <string>
#include <cassert>

namespace camy
{
	namespace hidden
	{
		extern std::ostream* g_output_stream;

		void set_output_stream(std::ostream* output_stream);
		std::ostream* get_output_stream();
		
		template <typename OutArg>
		void log_rec(const OutArg& arg);

		template <typename FirstOutArg, typename SecondOutArg, typename ...OutArgs>
		void log_rec(const FirstOutArg& first_out_arg, const SecondOutArg& second_out_arg, const OutArgs& ...out_args);

		template <typename FirstOutArg, typename ...OutArgs>
		void log(const char8* tag, const char8* function, const char8* line, const OutArgs& ...out_args);
	
		void debug_break();
	}
}

/*
	Tests if a state being set is malformed
*/
#if camy_flags & camy_validate_states
#define camy_validate_state(x, error_message) { if ((x) == nullptr) camy_warning(error_message); }
#else
#define camy_validate_state(...)
#endif

#if camy_flags & camy_enable_asserts
#define camy_assert(x, cmdlist, ...) { if (!(x)) (camy::hidden::debug_break()); }
#else
#define camy_assert(...)
#endif

#if camy_flags & camy_enable_logging_l1
#define camy_error(...) camy::hidden::log("Error", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#else
#define camy_error(...)
#endif

#if camy_flags & camy_enable_logging_l2
#define camy_warning(...) camy::hidden::log("Warning", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#else
#define camy_warning(...)
#endif

#if camy_flags & camy_enable_logging_l3
#define camy_info(...) camy::hidden::log("Info", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#else
#define camy_info(...)
#endif

#include "error.inl"