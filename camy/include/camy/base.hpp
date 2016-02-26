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
	Right now logging is done via simply writing to an output stream, something very neat will be implemented in the future
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
Topic: Platform
- OS: Windows 7 +
- Dependencies: D3D11, DXGI(1), Win32, D3DCompiler
- Language: C++11/14
- Compilers: VS2015 as for now, it should compile on mingw-w64 and possibly cygwin, but hasn't been tested


No additional linking directories should be added when opening this project in visual studio, the following "kits" should work:
- June 2010 DirectX SDK
- Windows 8.1
- Windows 10
*/

#if !defined(_MSC_VER)
#error Only visual studio 19.0+ on is supported ( C++ 14 features ), this code might run on other compilers, and if you \
	feel like modifying it do not worry about removing this error, some tweaking might be required
#endif

#if _MSC_VER < 1900
#error The cl compiler version doesn't have all the language features used, and code will almost certainly not work and \
	major rewrites would need to be done
#endif

#define camy_inline __forceinline

/*
	Topic: Common non-mathematical types
*/
#include <cstdlib>
#include <cstdint>

#define camy_to_type_ref(type, address) (*reinterpret_cast<type*>(address))

namespace camy
{
	// Types
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;

	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;
	using i64 = std::int64_t;

	using Byte = u8;

	using char8 = char;
	using char16 = char16_t;

	// Other types
	using Size = std::size_t;
	using PointerSize = std::intptr_t;

	template <typename Type>
	constexpr u8* offset(Type* ptr, PointerSize offset)noexcept
	{
		return reinterpret_cast<u8*>(ptr) + offset;
	}

	template <typename Type>
	constexpr const u8* offset(const Type* ptr, PointerSize offset)noexcept
	{
		return reinterpret_cast<const u8*>(ptr) + offset;
	}
}

// Forward declaration to avoid header bloating
#include "decls.hpp"

// Error utilities
#include "error.hpp"

/*
	Topic: Linking static libraries to static libraries. Keeping d3d11.lib and other libraries linked
			to camy gives a bunch of compiler warnings, what i'm doing then is to defined #pragma comment(lib), 
			this way camy automatically carries her dependencies with her.
			https://social.msdn.microsoft.com/Forums/en-US/5d79a108-6516-42d9-9626-05c622d2a007/want-to-fix-a-linker-warning?forum=winappswithnativecode
*/
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")