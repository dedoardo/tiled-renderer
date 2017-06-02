/* core.hpp
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
    namespace API
    {
        /*
         * All the values here are hardcoded because the internal code relies on them.
         * Modifying them will very likely make the code not compile as static_asserts
         * are scattered throughout the code whenever an assumption is made. They are used
         * mostly to avoid allocating dynamic memory and doing a couple of packing tricks.
         */
        constexpr rsize MAX_VERTEX_BUFFERS = 2;
        constexpr rsize MAX_RENDER_TARGETS = 2;
        constexpr rsize MAX_PARAMETER_BLOCKS = 5;
        constexpr rsize MAX_CONSTANT_BUFFERS = 6;
        constexpr rsize MAX_CONSTANT_BUFFER_SIZE = 2 << 16;
        constexpr rsize MAX_SHADER_RESOURCES = 2 << 7;
        constexpr rsize MAX_PASSES = 6; // Could be removed

        constexpr rsize MAX_CONTEXTS = 4;
    }

    using ContextID = std::remove_cv_t<decltype(API::MAX_CONTEXTS)>; // just to be fancy

    namespace API
    {
        constexpr ContextID INVALID_CONTEXT_ID = (ContextID)-1;
    }
}