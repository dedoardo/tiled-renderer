#pragma once

// camy
#include "../base.hpp"

namespace camy
{
	/*
	Struct: Page
	Represent a fixed size page of memory with a index indicating the current amount occupied
	*/
	template <Size page_size>
	struct Page
	{
		Byte buffer[page_size];

		Size next_free{ 0 };

		Page<page_size>* next{ nullptr };
		Page<page_size>* previous{ nullptr };
	};
}