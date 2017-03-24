/* d3d11_impl_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#if defined(camy_os_windows) && defined(camy_backend_d3d11)

#include "d3d11_graphics_base.hpp"
#include <camy/core/memory/vector.hpp>

// C++ STL
#include <atomic>
#include <thread>

namespace camy
{
	struct ConcurrentContextData
	{
		std::atomic_bool locked;
		ID3D11DeviceContext1* deferred_ctx = nullptr;
		std::thread::id owner;
	};

	struct camy_api RenderContextData
	{
		bool is_valid()const
		{
			return device != nullptr &&
				immediate_context != nullptr &&
				swap_chain != nullptr;
		}

		ConcurrentContextData contexts[kMaxConcurrentContexts];
		std::atomic_int avail_contexts;

		ID3D11Device*  device_old = nullptr;
		ID3D11Device1* device = nullptr;
		ID3D11DeviceContext* immediate_context_old = nullptr;
		ID3D11DeviceContext1* immediate_context = nullptr;
		IDXGIFactory* factory = nullptr;
		IDXGISwapChain* swap_chain = nullptr;
		IDXGIAdapter* adapter = nullptr;
        Surface      surface;
	};

	struct CommandListData;
	struct camy_api IncrementalCBufferInfo
	{
		// CBuffer currently being used
		ID3D11Buffer* cur_buffer = nullptr;
		byte*		  cur_data = nullptr;
		uint16		  prev_constants_off = 0;
		uint16		  constants_off = API::query(API::Query::MinConstantsPerUpdate);
		uint32		  next_idx = 0;

		void reset();
		ID3D11Buffer* alloc(CommandListData& cl_data, uint32 byte_size, const void* data);
		uint32 get_cur_constants_off()const;
	};

	struct camy_api CommandListData
	{
        ID3D11CommandList*    command_list = nullptr;
        ID3D11DeviceContext1* ctx = nullptr;
    };
}

#endif