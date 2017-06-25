/* d3d11_impl_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#if defined(CAMY_OS_WINDOWS) && defined(CAMY_BACKEND_D3D11)

// camy
#include <camy/camy.hpp>
#include <camy/system.hpp>

namespace camy
{
    struct ConcurrentContextData
    {
        uint32 locked;
        ID3D11DeviceContext1* deferred_ctx = nullptr;
        uint64 owner;
    };

    struct CAMY_API RenderContextData
    {
        bool is_valid() const
        {
            return device != nullptr && immediate_context != nullptr && swap_chain != nullptr;
        }

        ConcurrentContextData contexts[API::MAX_CONTEXTS];
        Atomic<uint32> avail_contexts;

        ID3D11Device* device_old = nullptr;
        ID3D11Device1* device = nullptr;
        ID3D11DeviceContext* immediate_context_old = nullptr;
        ID3D11DeviceContext1* immediate_context = nullptr;
        IDXGIFactory* factory = nullptr;
        IDXGISwapChain* swap_chain = nullptr;
        IDXGIAdapter* adapter = nullptr;
        Surface surface;
    };

    struct CAMY_API CommandListData
    {
        ID3D11CommandList* command_list = nullptr;
        ID3D11DeviceContext1* ctx = nullptr;
    };
}

#endif