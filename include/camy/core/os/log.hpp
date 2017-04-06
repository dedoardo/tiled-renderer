/* log.hpp
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

// C++ STL (only when logging is enabled)
#if camy_enable_logging > 0
#include <iostream>
#endif

/*!
	Logging facilities, did some testing w/ templates but macros 
	beat them hands down and would require too much trickery to get
	__FILE__ and __LINE__ to the callee w/o templates.

	I do admit that this method of logging might look kinda disgusting
	considering the way cleaner std::initializer_list approach, but this
	way arguments do not need to be converted to strings when passed to the
	log function, resulting in extremely cleaner logging code. In addition
	no code is generated when logging is disabled.
!*/
namespace camy
{
	namespace hidden
	{
		extern std::ostream* g_output_stream;
	}

	namespace API
	{
		void		  set_output_stream(std::ostream* output_stream);
		std::ostream* get_output_stream();
	}

	namespace hidden
	{
		//! Console control
		void set_info_color();
		void set_warning_color();
		void set_error_color();
		void restore_color();

		//! 
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

		camy_inline void log_header(const char8* tag, const char8* function, int line)
		{
			std::ostream& os = *g_output_stream;

			if (g_output_stream == &std::cout)
			{
				if (::camy::s_strcmp(tag, "Info"))
					set_info_color();
				else if (::camy::s_strcmp(tag, "Warn"))
					set_warning_color();
				else if (::camy::s_strcmp(tag, "Err "))
					set_error_color();
			}

			os << "[" << tag << "]";
		}

		camy_inline void log_footer(const char8* tag, const char8* function, int line)
		{
			std::ostream& os = *g_output_stream;

			// [namespace::to::func@line]
			os << " [" << function << "@" << line << "]";

			// Newline
			os << std::endl;

			// Previous color saved in header
			restore_color();
		}

#pragma warning(disable:4996)
		template <typename ...OutArgs>
		camy_api void log(const char8* tag, const char8* function, int line, const OutArgs& ...out_args)
		{
			if (g_output_stream == nullptr) return;
			log_header(tag, function, line);
			log_rec(out_args...);
			log_footer(tag, function, line);
		}

#pragma warning(default:4996)

		//! Here to avoid creating a new file just for a simple function
		void debug_break();
	}

	static const char8* kInfoTag = "Info";
	static const char8* kErrTag = "Err ";
	static const char8* kWarnTag = "Warn";

	namespace presets
	{
		using namespace hidden;
#define cl_preset_default_args const char8* function, int line

		template <typename T, typename ID>
		camy_api void system_err(cl_preset_default_args, const char8* system, const T& err, const ID& id)
		{
			if (g_output_stream == nullptr) return;
			std::ostream& os = *g_output_stream;

			log_header(kErrTag, function, line);
			os << "-SYSTEM ERROR-" << "Internal system: " << system << " ID: " << id << " failed with error code: " << err;
			log_footer(kErrTag, function, line);
		}

		template <typename ...Args>
		camy_api void internal_err(cl_preset_default_args, const Args& ...args)
		{
			if (g_output_stream == nullptr) return;
			std::ostream& os = *g_output_stream;

			log_header(kErrTag, function, line);
			os << "-INTERNAL ERROR-" << " Reason: ";
			hidden::log_rec(args...);
			log_footer(kErrTag, function, line);
		}

		camy_inline camy_api void create_err(cl_preset_default_args, const char8* what, const char8* name)
		{
			if (g_output_stream == nullptr) return;
			std::ostream& os = *g_output_stream;

			log_header(kErrTag, function, line);
			os << "-CREATION ERROR-" << "Failed to create " << what << "(" << name << ")";
			log_footer(kErrTag, function, line);
		}

		template <typename T>
		camy_api void invalid_arg(cl_preset_default_args, const char8* name, const T& val)
		{
			if (g_output_stream == nullptr) return;
			std::ostream& os = *g_output_stream;

			log_header(kErrTag, function, line);
			os << "-INVALID ARGUMENT-" << name << " = " << val;
			log_footer(kErrTag, function, line);
		}

		template <typename T>
		camy_api void invalid_arg_range(cl_preset_default_args, const char8* name, const T& val, const T& start, const T& end)
		{
			if (g_output_stream == nullptr) return;
			std::ostream& os = *g_output_stream;

			log_header(kErrTag, function, line);
			os << "-INVALID ARGUMENT-" << name << " = " << val << " valid range is [" << start << "," << end << "]";
			log_footer(kErrTag, function, line);
		}
	}
}

//! General
#if camy_enable_logging >= 3
#define cl_err(...) ::camy::hidden::log(::camy::kErrTag, __FUNCTION__, __LINE__,  __VA_ARGS__)
#else
#define camy_error(...)
#endif

#if camy_enable_logging >= 2
#define cl_warn(...) ::camy::hidden::log(::camy::kWarnTag, __FUNCTION__, __LINE__,  __VA_ARGS__)
#else
#define camy_warn(...)
#endif

#if camy_enable_logging >= 1
#define cl_info(...) ::camy::hidden::log(::camy::kInfoTag, __FUNCTION__, __LINE__,  __VA_ARGS__)
#else
#define camy_info(...)
#endif

#ifdef camy_enable_asserts
#define camy_assert(x) { if (!(x)) (camy::hidden::debug_break()); }
#else
#define camy_assert(x) camy_error("FAILED ASSERTION")
#endif

//! Presets
#define cl_system_err(system, code, id) ::camy::presets::system_err(__FUNCTION__, __LINE__, system, code, id)
#define cl_internal_err(...) ::camy::presets::internal_err(__FUNCTION__, __LINE__, __VA_ARGS__)
#define cl_create_err(what, name) ::camy::presets::create_err(__FUNCTION__, __LINE__, what, name)
#define cl_invalid_arg(var) ::camy::presets::invalid_arg(__FUNCTION__, __LINE__, #var, var)			
#define cl_invalid_arg_range(var, start_val, end_val) ::camy::presets::invalid_arg_range(__FUNCTION__, __LINE__, #var ,var, start_val, end_val);