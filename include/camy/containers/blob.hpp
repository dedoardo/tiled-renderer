/* blob.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core.hpp>

namespace camy
{
    struct CAMY_API Blob final
    {
        bool contains_data() const;
        void allocate(const byte* data, rsize size);
        void free();

        void* data = nullptr;
        rsize size = 0;
    };
}