/* base.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/base.hpp>

namespace camy
{
    bool Blob::contains_data() const
    {
        return byte_size > 0 && data != nullptr;
    }

    void Blob::allocate_data(const byte * in_data, rsize in_byte_size)
    {
        free_data();
        data = allocate(camy_loc, in_byte_size);
        if (in_data != nullptr)
            std::memcpy(data, in_data, in_byte_size);
        byte_size = in_byte_size;

    }

    void Blob::free_data()
    {
        deallocate(data);
        byte_size = 0;
    }
}