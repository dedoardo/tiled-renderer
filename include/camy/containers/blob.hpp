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
    // Container for raw data.
    //! Does not release data upon destruction
    //! Copy constructor and assignment operator COPY the data
    struct CAMY_API Blob final
    {
        Blob() = default;
        ~Blob() = default;

        Blob(const Blob& other);
        Blob(Blob&& other);

        Blob& operator=(const Blob& other);
        Blob& operator=(Blob&& other);

        // True if data != nullptr && size > 0
        bool contains_data() const;

        // Frees previously allocated data and makes space for size bytes.
        // If data != nullptr then the first size bytes of data are copied.
        void allocate(rsize size, const void* data = nullptr);

        // Frees previously allocated data. Nothing happens if no data is contained
        void free();

        void* data = nullptr;
        rsize size = 0;
    };
}