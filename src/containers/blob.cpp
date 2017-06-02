/* blob.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/containers/blob.hpp>

// camy
#include <camy/system.hpp>

namespace camy
{
	bool Blob::contains_data() const
	{
		return size > 0 && data != nullptr;
	}

	void Blob::allocate(const byte* in_data, rsize in_size)
	{
		free();
		data = API::allocate(CAMY_UALLOC(in_size));
		if (in_data != nullptr)
			memcpy(data, in_data, in_size);
		size = in_size;
	}

	void Blob::free()
	{
		API::deallocate(data);
		size = 0;
	}
}