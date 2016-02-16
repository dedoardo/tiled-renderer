#pragma once

// Macro for those parameters
#define camy_uuid_ptr(object) __uuidof(std::remove_pointer<decltype(object)>::type), reinterpret_cast<void**>(&object)

#include <type_traits>

struct IUnknown;

namespace camy
{
	template <typename Type>
	inline void safe_release(Type*& ptr)
	{
		// Delete on nullptr is perfectly valid
		delete ptr;
		ptr = nullptr;
	}

	template <typename Type>
	inline void safe_release_array(Type*& ptr)
	{
		// Delete on nullptr is perfectly valid
		delete[] ptr;
		ptr = nullptr;
	}

	template <typename Type>
	inline void safe_release_com(Type*& com_interface)
	{
		// Had to remove the static assert because didn't wanna bloat the headers including windows and just including IUknown includes window.s.h
		if (com_interface != nullptr)
		{
			com_interface->Release();
			com_interface = nullptr;
		}
	}
}