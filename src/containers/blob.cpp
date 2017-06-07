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
    Blob::Blob(const Blob& other) { allocate(other.size, other.data); }

    Blob::Blob(Blob&& other)
    {
        API::swap(data, other.data);
        API::swap(size, other.size);
    }

    Blob& Blob::operator=(const Blob& other)
    {
        free();
        allocate(other.size, other.data);
        return *this;
    }

    Blob& Blob::operator=(Blob&& other)
    {
        free();
        API::swap(data, other.data);
        API::swap(size, other.size);
        return *this;
    }

    bool Blob::contains_data() const { return size > 0 && data != nullptr; }

    void Blob::allocate(rsize in_size, const void* in_data)
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