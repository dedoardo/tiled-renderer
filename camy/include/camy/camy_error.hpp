#pragma once

// camy
#include "camy_base.hpp"

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

#if defined(camy_mode_final)
#define camy_assert(...)
#define camy_test_if(...)
#elif defined(camy_mode_test) // In test we stop the program at that point, when running in release we 
#define camy_assert(x, cmdlist, ...) { if (!(x)) (camy::hidden::debug_break()); }
#define camy_test_if(x, cmdlist ) { if ((x)) cmdlist; }
#else // When running release we output the error / warning without breakpointing the program
#define camy_assert(x, cmdlist, __VA_ARGS__) { if (!(x)) { cmdlist; camy_error(__VA_ARGS__)); } }
#endif


// Log functions are exposed via macros so that is it easy to turn them off / on based on kind of build
#if defined(camy_mode_final)
#define camy_info(...)
#define camy_warning(...)
#define camy_error(...)
#else // We just print in test/release mode
#define camy_info(...) camy::hidden::log("Info", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#define camy_warning(...) camy::hidden::log("Warning", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#define camy_error(...) camy::hidden::log("Error", __FUNCTION__, std::to_string(__LINE__).c_str(),  __VA_ARGS__)
#endif

#include "camy_error.inl"