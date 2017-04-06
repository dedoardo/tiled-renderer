/* base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

/*
	Topic: Camy Modes
	
	Camy can be compiled an run in two different modes: 
	- Test
	- Release
	We have more with different verbosity levels and such, verbosity can be either entirely toggled on 
	or toggled off. On Release mode is toggled off by default. Running in test mode adds *a lot* of information 
	to the data, often there are string and memory is tagged possibly making the whole thing run slower, but on the
	other hand errors and warnings are much easier to find. Other than data there are a lot of integrity checks that 
	are done. 
	A Mode is obviously determined at compile time, while verbosity can be turned off and turned on when running.
	Right now logging is done via simply writing to an output stream.
*/

/*
	Topic: Camy Errors
		
		All the camy functions return a simple boolean, a much more complicated system could be introduced, but 
		we prefer simplicity. The next question is thus : How do i handle grey, everything is not black and white 
		and something might go through but not entirely as expected, this is the very reason there is logging 
		facility where you can easily see what is not exactly going as planned.

		There are some operations that can bring to catastrophic results, such as accessing an array out of bounds
		or running out of memory. When this happens in non-critical parts of the code the operation is not continued
		and an error is returned to the user, some other times, it might happen inside some very critical code that 
		the user has no direct interface to it, making it a fatal error. If it's quite straightforward to prevent it,
		it is usually outputted to the error stream and the application is aborted with error informations. This might 
		sound very critical, but when it happens is for good reasons and it doesn't happen a lot ( if at all ).
*/

#if !defined(camy_compile_cpp)
#define camy_compile_cpp
#endif

/*
	OS:
	camy_os_windows
	camy_os_linux
camy_os_macosx
*/
#if defined(_WIN32)
#	define camy_os_windows
#else
#error Your OS is not currently supported
#endif


/*
	Defaulting to col-major
*/
#if !defined(camy_matrix_shader_order_row_major) && !defined(camy_matrix_shader_order_col_major)
#define camy_matrix_shader_order_col_major
#endif

#define camy_inline __forceinline
#if defined(camy_dll)
#   define camy_api __declspec(dllexport)
#else
#   define camy_api 
#endif
/*
	Note: compile flags
		camy_enable_states_validation	  - validate states prior to binding them
		camy_enable_layers_validation	  - enables naming of events and adds additional informations when logging ( + Graphics Debugger data )
		camy_enable_allocators_validation - Additional allocator validation ( TODO: Not currently implemented )
		camy_enable_asserts				  - Enables `camy_assert` that will trigger breakpoints
		camy_enable_logging				  - Enables facilities, 3 levels
												1 - Info	Successful completion of major operations
												2 - Warning Operation didnt go as expected
												3 - Error   Operation failed
		camy_enable_memory_tracking		  - Tracks all possible sources of memory allocation, including filename and line number

*/


/*
	Configuration modes are called test or final, but everything can be controlled by the
	above defines. If you want to modify default or add other modes simply add a case and
	define the required config parameters
*/
#if !defined(camy_mode_test) || !defined(camy_mode_final)
#	define camy_mode_test
#endif

#if defined(camy_mode_test)
#	define camy_enable_states_validation
#	define camy_enable_layers_validation
#	define camy_enable_allocators_validation
#	define camy_enable_asserts
#	define camy_enable_logging 3
#	define camy_enable_memory_tracking
#else
#	define camy_enable_logging 3
#endif

/*
	Topic: Common non-mathematical types
*/
#include <cstdlib>
#include <cstdint>

namespace camy
{
	// Types
	using uint8 = std::uint8_t;
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;
	using uint64 = std::uint64_t;

	using sint8 = std::int8_t;
	using sint16 = std::int16_t;
	using sint32 = std::int32_t;
	using sint64 = std::int64_t;

	using byte = uint8;
	using rsize = uint32;

	using char8 = char;
	using char16 = char16_t;

	// Used to have size = std::size_t but using uint64/uint32 is better
	using pointer_size = std::intptr_t;

	template <typename Type>
	camy_api inline void safe_release(Type*& ptr)
	{
		// Delete on nullptr is perfectly valid
		delete ptr;
		ptr = nullptr;
	}

	template <typename Type>
	camy_api inline void safe_release_array(Type*& ptr)
	{
		// Delete on nullptr is perfectly valid
		delete[] ptr;
		ptr = nullptr;
	}
}

#include "os/log.hpp"