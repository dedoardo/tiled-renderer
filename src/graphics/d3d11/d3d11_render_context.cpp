/* render_context.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/render_context.hpp>

#if defined(camy_os_windows) && defined(camy_backend_d3d11)

// camy
#include <camy/graphics/command_list.hpp>
#include <camy/core/memory/linear_vector.hpp>
#include <camy/core/memory/alloc.hpp>
#include <camy/graphics/platform.hpp>

// d3d11
#include <d3d11_1.h>
#include <d3dcommon.h>

// C++ STL
#include <thread>

template <typename ComType>
void safe_release_com(ComType*& ptr)
{
	if (ptr != nullptr)
		ptr->Release();
	ptr = nullptr;
}

void set_debug_name(ID3D11DeviceChild* child, const camy::char8* name)
{
	child->SetPrivateData(WKPDID_D3DDebugObjectName, ::camy::strlen(name), name);
}

namespace camy
{
	RenderContext::RenderContext() { }

	RenderContext::~RenderContext() { destroy(); }

	bool RenderContext::init(const StartupInfo& info_in, RuntimeInfo& info_out)
	{
		destroy();

		camy_info("Creating RenderContext..");
		camy_info("Num concurrent contexts: ", kMaxConcurrentContexts);

		HRESULT result = S_OK;
		result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_data.factory);
		if (FAILED(result))
		{
			camy_error("Failed to create IDXGIFactory: ", result);
			return false;
		}

		// Trying to create preferred device
		IDXGIAdapter* current_adapter = nullptr;
		if (m_data.factory->EnumAdapters(info_in.preferred_device, &current_adapter) != DXGI_ERROR_NOT_FOUND)
		{
            D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;

			UINT flags = 0x0;
#if defined(camy_enable_layers_validation)
			flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			D3D_FEATURE_LEVEL fl;
			result = D3D11CreateDevice(current_adapter, D3D_DRIVER_TYPE_UNKNOWN,
				nullptr, flags, &feature_level, 1, D3D11_SDK_VERSION,
				&m_data.device_old, &fl, &m_data.immediate_context_old);

			if (SUCCEEDED(result))
			{
				result = m_data.device_old->QueryInterface(__uuidof(ID3D11Device1),
					(void**)&m_data.device);

				if (SUCCEEDED(result))
				{
					m_data.immediate_context_old->QueryInterface(
						__uuidof(ID3D11DeviceContext1),
						(void**)&m_data.immediate_context);

					m_data.adapter = current_adapter;

					goto success;
				}

				// Device doesn't have 11.1 interface
				m_data.device->Release();
			}

			// Invalid adapter
			current_adapter->Release();
		}
		else
			camy_warning("Failed to create adapter on device: ", info_in.preferred_device);

		uint32 adapter_idx = 0;
		while (m_data.factory->EnumAdapters(adapter_idx, &current_adapter) != DXGI_ERROR_NOT_FOUND)
		{
			D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;
	
			UINT flags = 0x0;
#if defined(camy_enable_layers_validation)
			flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
			
			D3D_FEATURE_LEVEL fl;
			result = D3D11CreateDevice(current_adapter, D3D_DRIVER_TYPE_UNKNOWN,
				nullptr, flags, &feature_level, 1, D3D11_SDK_VERSION,
				&m_data.device_old, &fl, &m_data.immediate_context_old);

			if (SUCCEEDED(result))
			{
				result = m_data.device_old->QueryInterface(__uuidof(ID3D11Device1),
					(void**)&m_data.device);
				
				if (SUCCEEDED(result))
				{
					m_data.immediate_context_old->QueryInterface(
						__uuidof(ID3D11DeviceContext1),
						(void**)&m_data.immediate_context);

					m_data.adapter = current_adapter;

					goto success;
				}

				// Device doesn't have 11.1 interface
				m_data.device->Release();
			}

			// Invalid adapter
			current_adapter->Release();
            ++adapter_idx;
		}

		camy_error("Failed to find valid D3D11.1 Capable context");
		return false;

    success:
        RECT window_rect;
        GetClientRect((HWND)info_out.whandle, &window_rect); // GetWindowRect is not really correct and causes issued w/ alignment

        DXGI_SWAP_CHAIN_DESC sc_desc;
        sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        sc_desc.BufferDesc.Width = window_rect.right - window_rect.left;
        sc_desc.BufferDesc.Height = window_rect.bottom - window_rect.top;
        sc_desc.BufferDesc.RefreshRate.Numerator = 1;
        sc_desc.BufferDesc.RefreshRate.Denominator = 60;
        sc_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        sc_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

        UINT msaa = 1;
        UINT quality = 0;
        if (info_in.msaa > 1)
        {
            UINT quality;
            m_data.device->CheckMultisampleQualityLevels(sc_desc.BufferDesc.Format, info_in.msaa, &quality);
            if (quality == 0)
            {
                camy_warning("MSAA Level: ", info_in.msaa, " not supported, falling back to 1x");
            }
            else
            {
                sc_desc.SampleDesc.Count = info_in.msaa;
                sc_desc.SampleDesc.Quality = quality - 1;
            }
        }

        sc_desc.SampleDesc.Count = msaa;
        sc_desc.SampleDesc.Quality = quality;

        sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        sc_desc.BufferCount = 1;
        sc_desc.OutputWindow = (HWND)info_out.whandle;
        sc_desc.Windowed = true;
        sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sc_desc.Flags = 0;

        result = m_data.factory->CreateSwapChain(m_data.device, &sc_desc, &m_data.swap_chain);
        if (FAILED(result))
        {
            destroy();
            camy_error("Failed to create swap chain");
            return false;
        }

        Surface backbuffer;
        result = m_data.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer.native.texture2d);
        if (FAILED(result))
        {
            destroy();
            camy_error("Failed to obtain backbuffer");
            return false;
        }

        backbuffer.native.rtvs = tallocate_array<ID3D11RenderTargetView*>(camy_loc, 1, 16, nullptr);

        result = m_data.device->CreateRenderTargetView(backbuffer.native.texture2d, nullptr, &backbuffer.native.rtvs[0]);
        if (FAILED(result))
        {
            destroy();
            camy_error("Failed to create backbuffer rendertargetview");
            return false;
        }

        backbuffer.desc.width = sc_desc.BufferDesc.Width;
        backbuffer.desc.height = sc_desc.BufferDesc.Height;

        // Finally registering backbuffer
        m_backbuffer_handle = m_resource_manager.allocate<Surface>(camy_loc, "camy default window backbuffer");
        m_resource_manager.get<Surface>(m_backbuffer_handle) = backbuffer;

        for (int i = 0; i < kMaxConcurrentContexts; ++i)
        {
            m_data.device->CreateDeferredContext1(0, &m_data.contexts[i].deferred_ctx);
            m_data.contexts[i].locked = false;
        }
        m_data.avail_contexts = kMaxConcurrentContexts;

        // Filling out information
		info_out.backend = "D3D11.1";

		DXGI_ADAPTER_DESC adapter_desc;
		m_data.adapter->GetDesc(&adapter_desc);
		
		// It sucks I know 
		uint32 i = 0;
        while (i < info_out.gpu_name.max_len() && adapter_desc.Description[i] != (WCHAR)'\0')
        {
            info_out.gpu_name.append((char)adapter_desc.Description[i]);
            ++i;
        }

		info_out.dedicated_memory = (uint32)adapter_desc.DedicatedVideoMemory / (1024*1024);

		camy_info("Successfully created RenderContext: ");
		camy_info("Backend: ", info_out.backend);
		camy_info("Adapter: ", info_out.gpu_name);
		camy_info("VRAM: ", info_out.dedicated_memory, "MB");

		return true;
	}

	void RenderContext::destroy()
	{

	}

	void RenderContext::release(ContextID ctx_id)
	{

	}

	bool RenderContext::aquire(ContextID ctx_id)
	{
		assert(m_data.is_valid());
		
		int avail = m_data.avail_contexts;
		if (avail > 0 && m_data.avail_contexts.compare_exchange_strong(avail, avail - 1))
		{
			// Now we are absolutely sure there is a context that we can grab, just look for the one
			for (uint32 i = 0; i < kMaxConcurrentContexts; ++i)
			{
				bool des = false;
				if (m_data.contexts[i].locked.compare_exchange_strong(des, true))
				{
					m_data.contexts[i].owner = std::this_thread::get_id();
					camy_info("Successfully acquired concurrent RenderContext: ", ctx_id, " on thread: ", m_data.contexts[i].owner);
					return true;
				}
			}

			camy_assert(false);
		}

		camy_error("Failed to acquire context: ", ctx_id, " on thread: ", std::this_thread::get_id());
		return false;
	}

	ContextID RenderContext::get_id_for_current()
	{
		std::thread::id cur_id = std::this_thread::get_id();

		for (uint32 i = 0; i < kMaxConcurrentContexts; ++i)
		{
			if (m_data.contexts[i].locked && m_data.contexts[i].owner == cur_id)
				return i;
		}

		return kInvalidContextID;
	}

    RenderContextData& RenderContext::get_platform_data()
    {
        return m_data;
    }

	void RenderContext::flush(CommandList& command_list)
	{
		camy_assert(m_data.is_valid());

        if (command_list.m_data.command_list == nullptr)
            return;

		// Uploading cbuffer data
		for (rsize i = 0; i < command_list.m_updates.count(); ++i)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;

            CommandList::ConstantBufferUpdate& update = command_list.m_updates[i];
            ID3D11Buffer* cbuffer = get_constant_buffer(update.handle).native.buffer;

			m_data.immediate_context->Map(cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);

			// TODO: Is it worth for the last one to be partial ?
			std::memcpy(mapped_cbuffer.pData, update.data, update.bytes);

			m_data.immediate_context->Unmap(cbuffer, 0);
		}

		// Actually executing the command list
		m_data.immediate_context->ExecuteCommandList(command_list.m_data.command_list, false);   
        safe_release_com(command_list.m_data.command_list);
	}

    void RenderContext::immediate_clear()
    {
        m_data.immediate_context->ClearState();
        m_data.immediate_context->OMSetRenderTargets(1, get_backbuffer().native.rtvs, nullptr);
    }

	void RenderContext::clear_color(HResource target, const float4& color)
	{
		if (target == kInvalidHResource)
		{
			camy_warning("Tried to clear invalid resource");
			return;
		}

		Surface& surface = get_surface(target);
		m_data.immediate_context->ClearRenderTargetView(surface.native.rtvs[0], (float*)&color);
	}
	
	void RenderContext::clear_depth(HResource target)
	{
		assert(false); // TODO
	}

    HResource RenderContext::get_backbuffer_handle() const
    {
        return m_backbuffer_handle;
    }

    Surface& RenderContext::get_backbuffer()
    {
        return m_resource_manager.get<Surface>(m_backbuffer_handle);
    }

    void  RenderContext::swap_buffers()
    {
        camy_assert(m_data.is_valid());
        m_data.swap_chain->Present(1, 0);
    }

	DXGI_FORMAT camy_to_dxgi(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::Unknown:
			return DXGI_FORMAT_UNKNOWN;

			// Compressed formats
		case PixelFormat::BC1Unorm:
			return DXGI_FORMAT_BC1_UNORM;
		case PixelFormat::BC3Unorm:
			return DXGI_FORMAT_BC3_UNORM;

			// Typeless formats
        case PixelFormat::R8Typeless:
            return DXGI_FORMAT_R8_TYPELESS;
		case PixelFormat::R16Typeless:
			return DXGI_FORMAT_R16_TYPELESS;
		case PixelFormat::R32Typeless:
			return DXGI_FORMAT_R32_TYPELESS;
		case PixelFormat::R24G8Typeless:
			return DXGI_FORMAT_R24G8_TYPELESS;
		case PixelFormat::R24UnormX8Typeless:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

            // -> Single channel uncompressed
        case PixelFormat::R8Unorm:
            return DXGI_FORMAT_R8_UNORM;
        case PixelFormat::R16Unorm:
            return DXGI_FORMAT_R16_UNORM;
        case PixelFormat::R16Float:
            return DXGI_FORMAT_R16_FLOAT;
        case PixelFormat::R32Float:
            return DXGI_FORMAT_R32_FLOAT;

            // -> Two channels uncompressed
        case PixelFormat::RG8Unorm:
            return DXGI_FORMAT_R8G8_UNORM;
        case PixelFormat::RG16Unorm:
            return DXGI_FORMAT_R16G16_UNORM;
        case PixelFormat::RG16Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case PixelFormat::RG32Float:
            return DXGI_FORMAT_R32G32_FLOAT;

            // -> Four channels uncompressed
        case PixelFormat::RGBA8Unorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case PixelFormat::RGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

			// Depth formats
		case PixelFormat::D16Unorm:
			return DXGI_FORMAT_D16_UNORM;
		case PixelFormat::D32Float:
			return DXGI_FORMAT_D32_FLOAT;
		case PixelFormat::D24UNorm_S8Uint:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;

		default:
			camy_warning("Failed to translate format to backend API, not supported: ", (uint32)format);
			return DXGI_FORMAT_UNKNOWN;
		}
	}

    HResource RenderContext::create_surface(const SurfaceDesc& desc, const char8* name)
    {
        camy_assert(m_data.is_valid());

        ID3D11Texture2D* texture = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;

		D3D11_TEXTURE2D_DESC ndesc{ 0 };
        ndesc.Width = desc.width;
        ndesc.Height = desc.height;

        if (desc.type == SurfaceDesc::Type::Surface2D)
        {
            if (desc.surface_count > 1)
                camy_warning("Creating Surface2D with greater than 1 surface count, use Surface2DArray as type: ", name);
            ndesc.ArraySize = 1;
        }
        else if (desc.type == SurfaceDesc::Type::Surface2DArray)
        {
            if (desc.surface_count == 1)
                camy_warning("Creating Surface2DArray with 1 surface count, you might want to use Surface2D: ", name);
            ndesc.ArraySize = desc.surface_count;
        }
        else if (desc.type == SurfaceDesc::Type::SurfaceCube)
        {
            if (desc.surface_count > 1)
                camy_warning("Creating SurfaceCube with greater than 1 surface count, use SurfaceCubeArray as type: ", name);
            ndesc.ArraySize = 1;
        }
        else if (desc.type == SurfaceDesc::Type::SurfaceCubeArray)
        {
            if (desc.surface_count == 1)
                camy_warning("Creating SurfaceCubeArray with 1 surface count, you might want to use SurfaceCube: ", name);
            ndesc.ArraySize = 6 * desc.surface_count;
        }

        UINT msaa = 1;
        UINT quality = 0;
        if (desc.msaa_levels > 1)
        {
            UINT quality;
            m_data.device->CheckMultisampleQualityLevels(ndesc.Format, desc.msaa_levels, &quality);
            if (quality == 0)
            {
                camy_warning("MSAA Level: ", desc.msaa_levels, " not supported, falling back to 1x");
            }
            else
            {
                ndesc.SampleDesc.Count = desc.msaa_levels;
                ndesc.SampleDesc.Quality = quality - 1;
            }
        }

        ndesc.SampleDesc.Count = msaa;
        ndesc.SampleDesc.Quality = quality;

        ndesc.MipLevels = desc.mip_levels;
        ndesc.Format = camy_to_dxgi(desc.pixel_format);
        ndesc.Usage = desc.is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        ndesc.CPUAccessFlags = desc.is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        ndesc.BindFlags = 0;
        if (desc.gpu_views & GPUView_ShaderResource)
            ndesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if (desc.gpu_views & GPUView_RenderTarget)
            ndesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        if (desc.gpu_views & GPUView_DepthStencil)
            ndesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        if (desc.gpu_views & GPUView_UnorderedAccess)
            ndesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        ndesc.MiscFlags = 0;
        if (desc.type == SurfaceDesc::Type::SurfaceCube ||
            desc.type == SurfaceDesc::Type::SurfaceCubeArray)
            ndesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;


        uint32 num_subresources = 1;
        if (desc.type == SurfaceDesc::Type::SurfaceCube)
            num_subresources *= 6;
        if (desc.type == SurfaceDesc::Type::SurfaceCubeArray ||
            desc.type == SurfaceDesc::Type::Surface2DArray)
            num_subresources *= desc.surface_count;

        LinearVector<D3D11_SUBRESOURCE_DATA> sub_resources(num_subresources);
        if (!desc.initial_subsurface_data.empty())
        {
            for (uint32 i = 0; i < num_subresources; ++i)
            {
                D3D11_SUBRESOURCE_DATA& next = sub_resources.next();
                next.pSysMem = desc.initial_subsurface_data[i].data;
                next.SysMemPitch = desc.initial_subsurface_data[i].pitch;
                next.SysMemSlicePitch = 0;
            }
        }

        // Creating views
        uint32 num_views = 1;
        if (desc.type == SurfaceDesc::Type::Surface2DArray ||
            desc.type == SurfaceDesc::Type::SurfaceCubeArray)
        {
            num_views *= desc.surface_count;
        }

        if (desc.type == SurfaceDesc::Type::SurfaceCube ||
            desc.type == SurfaceDesc::Type::SurfaceCubeArray)
        {
            num_views *= 6;
        }

        ID3D11ShaderResourceView** srvs = nullptr;
        ID3D11RenderTargetView** rtvs = nullptr;
        ID3D11UnorderedAccessView** uavs = nullptr;
        ID3D11DepthStencilView** dsvs = nullptr;

        HRESULT result = m_data.device->CreateTexture2D(&ndesc, desc.initial_subsurface_data.empty() ? nullptr : sub_resources.data(), &texture);
        if (FAILED(result))
        {
            camy_error("Failed to create Surface: ", name);
            goto error;
        }

        if (desc.gpu_views & GPUView_ShaderResource)
        {
            srvs = tallocate_array<ID3D11ShaderResourceView*>(camy_loc, num_views, 16, nullptr);

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
            srv_desc.Format = camy_to_dxgi(desc.pixel_format_srv);

            for (uint32 i = 0; i < num_views; ++i)
            {
                switch (desc.type)
                {
                case SurfaceDesc::Type::Surface2D:
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MipLevels = desc.mip_levels;
                    srv_desc.Texture2D.MostDetailedMip = 0;

                    break;
                case SurfaceDesc::Type::Surface2DArray:
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    srv_desc.Texture2DArray.MostDetailedMip = 0;
                    srv_desc.Texture2DArray.MipLevels = desc.mip_levels;
                    srv_desc.Texture2DArray.FirstArraySlice = i;
                    srv_desc.Texture2DArray.ArraySize = 1;

                    break;
                case SurfaceDesc::Type::SurfaceCube:
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    srv_desc.TextureCube.MipLevels = desc.mip_levels;
                    srv_desc.TextureCube.MostDetailedMip = 0;

                    break;
                case SurfaceDesc::Type::SurfaceCubeArray:
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srv_desc.TextureCubeArray.MostDetailedMip = 0;
                    srv_desc.TextureCubeArray.MipLevels = desc.mip_levels;
                    srv_desc.TextureCubeArray.First2DArrayFace = i;
                    srv_desc.TextureCubeArray.NumCubes = 1;
                    break;
                }

                if (FAILED(m_data.device->CreateShaderResourceView(texture, &srv_desc, &srvs[i])))
                {
                    camy_error("Failed to create GPUView_ShaderResource for surface: ", name, ":", i);
                    goto error;
                }
            }
        }

        if (desc.gpu_views & GPUView_RenderTarget)
        {
            rtvs = tallocate_array<ID3D11RenderTargetView*>(camy_loc, num_views, 16, nullptr);

            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
            rtv_desc.Format = camy_to_dxgi(desc.pixel_format_rtv);

            for (uint32 i = 0; i < num_views; ++i)
            {
                switch (desc.type)
                {
                case SurfaceDesc::Type::Surface2D:
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtv_desc.Texture2D.MipSlice = 0;
                    break;
                case SurfaceDesc::Type::Surface2DArray:
                case SurfaceDesc::Type::SurfaceCube:
                case SurfaceDesc::Type::SurfaceCubeArray: // TODO: have yet to test surfacecubearray
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtv_desc.Texture2DArray.MipSlice = 0;
                    rtv_desc.Texture2DArray.FirstArraySlice = i;
                    rtv_desc.Texture2DArray.ArraySize = 1;
                    break;
                default:
                    camy_assert(false);
                }

                if (FAILED(m_data.device->CreateRenderTargetView(texture, &rtv_desc, &rtvs[i])))
                {
                    camy_error("Failed to create GPUView_RenderTarget for surface: ", name, ":", i);
                    goto error;
                }
            }
        }

        if (desc.gpu_views & GPUView_UnorderedAccess)
        {
            uavs = tallocate_array<ID3D11UnorderedAccessView*>(camy_loc, num_views, 16, nullptr);

            D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
            uav_desc.Format = camy_to_dxgi(desc.pixel_format_uav);
            for (uint32 i = 0; i < num_views; ++i)
            {
                switch (desc.type)
                {
                case SurfaceDesc::Type::Surface2D:
                    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    uav_desc.Texture2D.MipSlice = 0;
                    break;
                case SurfaceDesc::Type::Surface2DArray:
                case SurfaceDesc::Type::SurfaceCube:
                case SurfaceDesc::Type::SurfaceCubeArray: // TODO: have yet to test surfacecubearray
                    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    uav_desc.Texture2DArray.MipSlice = 0;
                    uav_desc.Texture2DArray.FirstArraySlice = i;
                    uav_desc.Texture2DArray.ArraySize = 1;
                }

                if (FAILED(m_data.device->CreateUnorderedAccessView(texture, &uav_desc, &uavs[i])))
                {
                    camy_error("Failed to create GPUView_UnorderedAccess for surface: ", name, ":", i);
                    goto error;
                }
            }
        }

        if (desc.gpu_views & GPUView_DepthStencil)
        {
            dsvs = tallocate_array<ID3D11DepthStencilView*>(camy_loc, num_views, 16, nullptr);

            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
            dsv_desc.Format = camy_to_dxgi(desc.pixel_format_uav);
            for (uint32 i = 0; i < num_views; ++i)
            {
                switch (desc.type)
                {
                case SurfaceDesc::Type::Surface2D:
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsv_desc.Texture2D.MipSlice = 0;
                    break;
                case SurfaceDesc::Type::Surface2DArray:
                case SurfaceDesc::Type::SurfaceCube:
                case SurfaceDesc::Type::SurfaceCubeArray: // TODO: have yet to test surfacecubearray
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsv_desc.Texture2DArray.MipSlice = 0;
                    dsv_desc.Texture2DArray.FirstArraySlice = i;
                    dsv_desc.Texture2DArray.ArraySize = 1;
                }

                if (FAILED(m_data.device->CreateDepthStencilView(texture, &dsv_desc, &dsvs[i])))
                {
                    camy_error("Failed to create GPUView_DepthStencil for surface: ", name, ":", i);
                    goto error;
                }
            }
        }

        if (name != nullptr)
            set_debug_name(texture, name);

        HResource ret = m_resource_manager.allocate<Surface>(camy_loc, name);
        Surface& res = m_resource_manager.get<Surface>(ret);

        res.desc = desc;
        res.native.srvs = srvs;
        res.native.rtvs = rtvs;
        res.native.uavs = uavs;
        res.native.dsvs = dsvs;

        camy_info("Successfully created Surface: ", name, "[", desc.width, "x", desc.height, "]");
        return ret;

    error:
        tdeallocate(dsvs);
        tdeallocate(uavs);
        tdeallocate(rtvs);
        tdeallocate(srvs);
        safe_release_com(texture);
        return kInvalidHResource;
    }

    HResource RenderContext::create_buffer(const BufferDesc& desc, const char8* name)
    {
        camy_assert(m_data.is_valid());

        ID3D11Buffer* buffer = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
		ID3D11UnorderedAccessView* uav = nullptr;

        // Resource
        if (!(desc.gpu_views & GPUView_ShaderResource))
            camy_warning("Can't create a buffer not as shader resource");

        D3D11_BUFFER_DESC ndesc;
        ndesc.ByteWidth = desc.num_elements * desc.element_size;
        ndesc.Usage = D3D11_USAGE_DYNAMIC;
        ndesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        if (desc.gpu_views & GPUView_UnorderedAccess)
        {
            ndesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            ndesc.Usage = D3D11_USAGE_DEFAULT;
        }
        ndesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ndesc.StructureByteStride = desc.element_size;

        UINT misc_flags = 0;
        if (desc.type == BufferDesc::Type::Structured)
            misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        ndesc.MiscFlags = misc_flags;

        HRESULT result = m_data.device->CreateBuffer(&ndesc, nullptr, &buffer);
        if (FAILED(result))
        {
            camy_error("Failed to create buffer: ", result);
            goto error;
        }

        // Shader Resource View
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.ElementOffset = 0;
        srv_desc.Buffer.NumElements = desc.num_elements;

        result = m_data.device->CreateShaderResourceView(buffer, &srv_desc, &srv);
        if (FAILED(result))
        {
            camy_error("Failed to create GPUView_ShaderResource: ", result);
            goto error;
        }

        // Unordered access view
        if (desc.gpu_views & GPUView_UnorderedAccess)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
            uav_desc.Format = DXGI_FORMAT_UNKNOWN;
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uav_desc.Buffer.FirstElement = 0;
            uav_desc.Buffer.NumElements = desc.num_elements;
            uav_desc.Buffer.Flags = 0;

            result = m_data.device->CreateUnorderedAccessView(buffer, &uav_desc, &uav);
            if (FAILED(result))
            {
                camy_error("Failed to create GPUView_UnorderedAccess: ", result);
                goto error;
            }
        }
        if (name != nullptr)
            set_debug_name(buffer, name);

        HResource ret = m_resource_manager.allocate<Buffer>(camy_loc, name);
        Buffer& res = m_resource_manager.get<Buffer>(ret);

        res.desc = desc;
        res.native.buffer = buffer;
        res.native.srv = srv;
        res.native.uav = uav;

        camy_info("Successfully created buffer: ", name, "[", desc.element_size, "x", desc.num_elements, "]");
        return ret;
    error:
        safe_release_com(uav);
        safe_release_com(srv);
        safe_release_com(buffer);
        return kInvalidHResource;
    }

    HResource RenderContext::create_vertex_buffer(const VertexBufferDesc& desc, const char8* name)
    {
        camy_assert(m_data.is_valid());

        ID3D11Buffer* buffer = nullptr;

        D3D11_BUFFER_DESC ndesc;
        ndesc.ByteWidth = desc.num_elements * desc.element_size;
        ndesc.Usage = desc.is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        ndesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        ndesc.CPUAccessFlags = desc.is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        ndesc.MiscFlags = 0;
        ndesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vb_data;
        vb_data.pSysMem = desc.initial_data;
        vb_data.SysMemPitch = vb_data.SysMemSlicePitch = 0;

        HRESULT result = m_data.device->CreateBuffer(&ndesc, (desc.initial_data != nullptr) ? &vb_data : nullptr, &buffer);
        if (FAILED(result))
        {
            camy_error("Faile to create buffer: ", name);
            goto error;
        }

        if (name == nullptr)
            set_debug_name(buffer, name);

        HResource ret = m_resource_manager.allocate<VertexBuffer>(camy_loc, name);
        VertexBuffer& res = m_resource_manager.get<VertexBuffer>(ret);

        res.desc = desc;
        res.native.buffer = buffer;
        res.native.stride = desc.element_size;

        camy_info("Successfully created vertex buffer: ", name);
        return ret;

    error:
        safe_release_com(buffer);
        return kInvalidHResource;
    }

    HResource RenderContext::create_index_buffer(const IndexBufferDesc& desc, const char8* name)
    {
        camy_assert(m_data.is_valid());

        ID3D11Buffer* buffer = nullptr;

        D3D11_BUFFER_DESC ndesc;
        ndesc.ByteWidth = desc.num_elements * desc.element_size;
        ndesc.Usage = desc.is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        ndesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ndesc.CPUAccessFlags = desc.is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        ndesc.MiscFlags = 0;
        ndesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vb_data;
        vb_data.pSysMem = desc.initial_data;
        vb_data.SysMemPitch = vb_data.SysMemSlicePitch = 0;

        HRESULT result = m_data.device->CreateBuffer(&ndesc, (desc.initial_data != nullptr) ? &vb_data : nullptr, &buffer);
        if (FAILED(result))
        {
            camy_error("Faile to create buffer: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(buffer, name);

        HResource ret = m_resource_manager.allocate<IndexBuffer>(camy_loc, name);
        IndexBuffer& res = m_resource_manager.get<IndexBuffer>(ret);

        res.desc = desc;
        res.native.buffer = buffer;
        if (desc.element_size == 2)
            res.native.dxgi_format = DXGI_FORMAT_R16_UINT;
        else
            res.native.dxgi_format = DXGI_FORMAT_R32_UINT;

        camy_info("Successfully created index buffer: ", name, "[", desc.element_size, "x", desc.num_elements, "]");
        return ret;

    error:
        safe_release_com(buffer);
        return kInvalidHResource;
    }

    HResource RenderContext::create_constant_buffer(const ConstantBufferDesc& desc, const char8* name)
    {
        camy_assert(m_data.is_valid());

        ID3D11Buffer* buffer = nullptr;

        D3D11_BUFFER_DESC ndesc;
        ndesc.ByteWidth = desc.size;
        ndesc.Usage = D3D11_USAGE_DYNAMIC;
        ndesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ndesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ndesc.MiscFlags = 0;
        ndesc.StructureByteStride = 0;

        HRESULT result = m_data.device->CreateBuffer(&ndesc, nullptr, &buffer);
        if (FAILED(result))
        {
            camy_error("Failed to create constant buffer: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(buffer, name);

        HResource ret = m_resource_manager.allocate<ConstantBuffer>(camy_loc, name);
        ConstantBuffer& res = m_resource_manager.get<ConstantBuffer>(ret);
        
        res.desc = desc;
        res.native.buffer = buffer;

        camy_info("Successfully create ConstantBuffer: ", name, "[", desc.size, "]");
        return ret;
    error:
        safe_release_com(buffer);
        return kInvalidHResource;
    }
	
    void compile_from_camy(BlendStateDesc::Type blend_mode, D3D11_BLEND_DESC& bs_desc)
    {
        bs_desc.AlphaToCoverageEnable = false; // NO MSAA support yet
        bs_desc.IndependentBlendEnable = false; // Not supported yet
        bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        switch (blend_mode)
        {
        case BlendStateDesc::Type::Opaque:
            bs_desc.RenderTarget[0].BlendEnable = true;
            bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
            bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            break;

        case BlendStateDesc::Type::Transparent:
            bs_desc.RenderTarget[0].BlendEnable = true;
            bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            break;

        case BlendStateDesc::Type::Additive:
            bs_desc.RenderTarget[0].BlendEnable = true;
            bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            break;
        default:
            camy_assert(false);
        }
    }

    D3D11_FILL_MODE camy_to_d3d11(RasterizerStateDesc::Fill fill_mode)
    {
        switch (fill_mode)
        {
        case RasterizerStateDesc::Fill::Solid:
            return D3D11_FILL_SOLID;
        case RasterizerStateDesc::Fill::Wireframe:
            return D3D11_FILL_WIREFRAME;
        default:
            return D3D11_FILL_SOLID;
        }
    }

    D3D11_CULL_MODE camy_to_d3d11(RasterizerStateDesc::Cull cull_mode)
    {
        switch (cull_mode)
        {
        case RasterizerStateDesc::Cull::Back:
            return D3D11_CULL_BACK;
        case RasterizerStateDesc::Cull::Front:
            return D3D11_CULL_FRONT;
        case RasterizerStateDesc::Cull::None:
            return D3D11_CULL_NONE;
        default:
            return D3D11_CULL_NONE; // Default d3d11 is back but None IMHO Makes the user notice that there is something wrong :)
        }
    }

    HResource RenderContext::create_blend_state(const BlendStateDesc& desc, const char8* name)
    {
        camy_assert(m_data.is_valid());

        D3D11_BLEND_DESC bs_desc;
        compile_from_camy(desc.type, bs_desc);

        ID3D11BlendState* blend_state = nullptr;
        if (FAILED(m_data.device->CreateBlendState(&bs_desc, &blend_state)))
        {
            camy_error("Failed to create BlendState: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(blend_state, name);

        HResource ret = m_resource_manager.allocate<BlendState>(camy_loc, name);
        BlendState& res = m_resource_manager.get<BlendState>(ret);

        res.desc = desc;
        res.native.state = blend_state;

        camy_info("Successfully created BlendState");
        return ret;

    error:
        safe_release_com(blend_state);
        return kInvalidHResource;
    }

    HResource RenderContext::create_rasterizer_state(const RasterizerStateDesc& desc, const char8* name)
    {
        camy_assert(m_data.device != nullptr);

        D3D11_RASTERIZER_DESC rs_desc;
        rs_desc.CullMode = camy_to_d3d11(desc.cull);
        rs_desc.FillMode = camy_to_d3d11(desc.fill);
        rs_desc.FrontCounterClockwise = false;
        rs_desc.DepthBias = desc.depth_bias;
        rs_desc.DepthBiasClamp = desc.depth_bias_clamp;
        rs_desc.SlopeScaledDepthBias = desc.slope_scaled_depth_bias;
        rs_desc.DepthClipEnable = true;
        rs_desc.ScissorEnable = false;
        rs_desc.MultisampleEnable = true;
        rs_desc.AntialiasedLineEnable = false;

        ID3D11RasterizerState* rasterizer_state = nullptr;
        if (FAILED(m_data.device->CreateRasterizerState(&rs_desc, &rasterizer_state)))
        {
            camy_error("Failed to create RasterizerState: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(rasterizer_state, name);

        HResource ret = m_resource_manager.allocate<RasterizerState>(camy_loc, name);
        RasterizerState& res = m_resource_manager.get<RasterizerState>(ret);

        res.desc = desc;
        res.native.state = rasterizer_state;

        camy_info("Successfully created RasterizerState: ", name);
        return ret;

    error:
        safe_release_com(rasterizer_state);
        return kInvalidHResource;
    }

    DXGI_FORMAT camy_to_d3d11(InputElement::Type type)
    {
        switch (type)
        {
        case InputElement::Type::Float3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case InputElement::Type::Float4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    HResource RenderContext::create_input_signature(const InputSignatureDesc& desc, const char8* name)
    {
        camy_assert(m_data.device != nullptr);

        LinearVector<D3D11_INPUT_ELEMENT_DESC> input_elements(desc.num_elements);
        for (unsigned int i = 0; i < desc.num_elements; ++i)
        {
            D3D11_INPUT_ELEMENT_DESC& next = input_elements.next();
            next.SemanticName = desc.elements[i].name.str();
            next.SemanticIndex = desc.elements[i].semantic_idx;
            next.InputSlot = desc.elements[i].slot;
            next.Format = camy_to_d3d11(desc.elements[i].type);

            if (desc.elements[i].is_instanced)
            {
                next.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                next.InstanceDataStepRate = 1;
            }
            else
            {
                next.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                next.InstanceDataStepRate = 0;
            }
        }

        ID3D11InputLayout* input_layout = nullptr;
        if (FAILED(m_data.device->CreateInputLayout(input_elements.data(), desc.num_elements, desc.bytecode.data, desc.bytecode.byte_size, &input_layout)))
        {
            camy_error("Failed to create InputSignature: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(input_layout, name);

        HResource ret = m_resource_manager.allocate<InputSignature>(camy_loc, name);
        InputSignature& res = m_resource_manager.get<InputSignature>(ret);

        res.desc = desc;
        res.native.input_layout = input_layout;

        camy_info("Successfully create InputSignature: ", name);
        return ret;

    error:
        safe_release_com(input_layout);
        return kInvalidHResource;
    }

    D3D11_TEXTURE_ADDRESS_MODE camy_to_d3d11(SamplerDesc::Address address_mode)
    {
        switch (address_mode)
        {
        case SamplerDesc::Address::Clamp:
            return D3D11_TEXTURE_ADDRESS_CLAMP;
        case SamplerDesc::Address::Wrap:
            return D3D11_TEXTURE_ADDRESS_WRAP;
        case SamplerDesc::Address::Mirror:
            return D3D11_TEXTURE_ADDRESS_MIRROR;
        default:
            return D3D11_TEXTURE_ADDRESS_CLAMP;
        }
    }

    D3D11_FILTER camy_to_d3d11(SamplerDesc::Filter filter, bool comparison)
    {
        switch (filter)
        {
        case SamplerDesc::Filter::Point:
            return comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
        case SamplerDesc::Filter::Linear:
            return comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        case SamplerDesc::Filter::Anisotropic:
            return comparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
        default:
            return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        }
    }

    D3D11_COMPARISON_FUNC camy_to_d3d11(SamplerDesc::Comparison comparison)
    {
        switch (comparison)
        {
        case SamplerDesc::Comparison::Less:
            return D3D11_COMPARISON_LESS;
        case SamplerDesc::Comparison::LessEqual:
            return D3D11_COMPARISON_LESS_EQUAL;
        case SamplerDesc::Comparison::Never:
            return D3D11_COMPARISON_NEVER;
        default:
            return D3D11_COMPARISON_NEVER;
        }
    }

    HResource RenderContext::create_sampler(const SamplerDesc& desc, const char8* name)
    {
        camy_assert(m_data.device != nullptr);

        D3D11_SAMPLER_DESC s_desc;
        s_desc.Filter = camy_to_d3d11(desc.filter, desc.comparison != SamplerDesc::Comparison::Never);
        s_desc.AddressU =
            s_desc.AddressV =
            s_desc.AddressW = camy_to_d3d11(desc.address);
        s_desc.MinLOD = -FLT_MAX;
        s_desc.MaxLOD = FLT_MAX;
        s_desc.MipLODBias = 0.f;
        s_desc.MaxAnisotropy = desc.filter == SamplerDesc::Filter::Anisotropic ? D3D11_MAX_MAXANISOTROPY : 0;
        s_desc.ComparisonFunc = camy_to_d3d11(desc.comparison);
        s_desc.BorderColor[0] =
            s_desc.BorderColor[1] =
            s_desc.BorderColor[2] =
            s_desc.BorderColor[3] = 1.f;

        ID3D11SamplerState* sampler;
        if (FAILED(m_data.device->CreateSamplerState(&s_desc, &sampler)))
        {
            camy_error("Failed to create Sampler: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(sampler, name);

        HResource ret = m_resource_manager.allocate<Sampler>(camy_loc, name);
        Sampler& res = m_resource_manager.get<Sampler>(ret);

        res.desc = desc;
        res.native.sampler = sampler;

        camy_info("Successfully created Sampler: ", name);
        return ret;

    error:
        sampler = nullptr;
        return kInvalidHResource;
    }

    HResource RenderContext::create_depth_stencil_state(const DepthStencilStateDesc& desc, const char8* name)
    {
        camy_assert(m_data.device != nullptr);

        D3D11_DEPTH_STENCIL_DESC dss_desc;
        dss_desc.DepthEnable = true;
        dss_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dss_desc.DepthFunc = D3D11_COMPARISON_LESS;
        dss_desc.StencilEnable = false;

        ID3D11DepthStencilState* depth_stencil_state = nullptr;
        if (FAILED(m_data.device->CreateDepthStencilState(&dss_desc, &depth_stencil_state)))
        {
            camy_error("Failed to create DepthStencilState: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(depth_stencil_state, name);

        HResource ret = m_resource_manager.allocate<DepthStencilState>(camy_loc, name);
        DepthStencilState& res = m_resource_manager.get<DepthStencilState>(ret);

        res.desc = desc;
        res.native.state = depth_stencil_state;

        camy_info("Successfully created DepthStencilState: ", name);
        return ret;

    error:
        safe_release_com(depth_stencil_state);
        return kInvalidHResource;
    }

    HResource RenderContext::create_shader(const ShaderDesc& desc, const char8* name)
    {
        camy_assert(m_data.device != nullptr);

        ID3D11DeviceChild* shader = nullptr;
        HRESULT result = S_FALSE;
        switch (desc.type)
        {
        case ShaderDesc::Type::Vertex:
            result = m_data.device->CreateVertexShader(desc.bytecode.data, desc.bytecode.byte_size, nullptr, (ID3D11VertexShader**)&shader);
            break;
        case ShaderDesc::Type::Geometry:
            result = m_data.device->CreateGeometryShader(desc.bytecode.data, desc.bytecode.byte_size, nullptr, (ID3D11GeometryShader**)&shader);
            break;
        case ShaderDesc::Type::Pixel:
            result = m_data.device->CreatePixelShader(desc.bytecode.data, desc.bytecode.byte_size, nullptr, (ID3D11PixelShader**)&shader);
            break;
        case ShaderDesc::Type::Compute:
            result = m_data.device->CreateComputeShader(desc.bytecode.data, desc.bytecode.byte_size, nullptr, (ID3D11ComputeShader**)&shader);
            break;
        default:
            camy_assert(false);
        }

        if (FAILED(result))
        {
            camy_error("Failed to create Shader: ", name);
            goto error;
        }

        if (name != nullptr)
            set_debug_name(shader, name);

        HResource ret = m_resource_manager.allocate<Shader>(camy_loc, name);
        Shader& res = m_resource_manager.get<Shader>(ret);

        res.desc = desc;
        res.native.shader = shader;

        camy_info("Successfully created Shader: ", name);
        return ret;

    error:
        safe_release_com(shader);
        return kInvalidHResource;
    }

	void RenderContext::destroy_surface(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        Surface& surface = m_resource_manager.get<Surface>(handle);

		if (surface.native.srvs != nullptr)
		{
			for (rsize i = 0; i < surface.native.num_views; ++i)
				safe_release_com(surface.native.srvs[i]);
		}

		if (surface.native.rtvs != nullptr)
		{
			for (rsize i = 0; i < surface.native.num_views; ++i)
				safe_release_com(surface.native.rtvs[i]);
		}

		if (surface.native.uavs != nullptr)
		{
			for (rsize i = 0; i < surface.native.num_views; ++i)
				safe_release_com(surface.native.uavs[i]);
		}

		if (surface.native.dsvs != nullptr)
		{
			for (rsize i = 0; i < surface.native.num_views; ++i)
				safe_release_com(surface.native.dsvs[i]);
		}

        m_resource_manager.deallocate<Surface>(handle);
	}

	void RenderContext::destroy_buffer(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        Buffer& buffer = m_resource_manager.get<Buffer>(handle);

		safe_release_com(buffer.native.uav);
		safe_release_com(buffer.native.srv);
		safe_release_com(buffer.native.buffer);
		
        m_resource_manager.deallocate<Buffer>(handle);
	}

	void RenderContext::destroy_vertex_buffer(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        VertexBuffer& vertex_buffer = m_resource_manager.get<VertexBuffer>(handle);

		safe_release_com(vertex_buffer.native.buffer);
	
        m_resource_manager.deallocate<VertexBuffer>(handle);
    }

	void RenderContext::destroy_index_buffer(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        IndexBuffer& index_buffer = m_resource_manager.get<IndexBuffer>(handle);

		safe_release_com(index_buffer.native.buffer);
            
        m_resource_manager.deallocate<IndexBuffer>(handle);
    }

	void RenderContext::destroy_constant_buffer(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        ConstantBuffer& constant_buffer = m_resource_manager.get<ConstantBuffer>(handle);
        
        safe_release_com(constant_buffer.native.buffer);
	
        m_resource_manager.deallocate<ConstantBuffer>(handle);
    }

	void RenderContext::destroy_blend_state(HResource handle)
    {
        if (handle == kInvalidHResource) return;
        BlendState& blend_state = m_resource_manager.get<BlendState>(handle);

		safe_release_com(blend_state.native.state);
	
        m_resource_manager.deallocate<BlendState>(handle);
    }

	void RenderContext::destroy_rasterizer_state(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        RasterizerState& rasterizer_state = m_resource_manager.get<RasterizerState>(handle);

		safe_release_com(rasterizer_state.native.state);
	
        m_resource_manager.deallocate<RasterizerState>(handle);
    }

	void RenderContext::destroy_input_signature(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        InputSignature& input_signature = m_resource_manager.get<InputSignature>(handle);

		safe_release_com(input_signature.native.input_layout);
	    
        m_resource_manager.deallocate<InputSignature>(handle);
    }

	void RenderContext::destroy_sampler(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        Sampler& sampler = m_resource_manager.get<Sampler>(handle);

		safe_release_com(sampler.native.sampler);
	
        m_resource_manager.deallocate<Sampler>(handle);
    }

	void RenderContext::destroy_depth_stencil_state(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        DepthStencilState& depth_stencil_state = m_resource_manager.get<DepthStencilState>(handle);

		safe_release_com(depth_stencil_state.native.state);
	    
        m_resource_manager.deallocate<DepthStencilState>(handle);
    }

	void RenderContext::destroy_shader(HResource handle)
	{
        if (handle == kInvalidHResource) return;
        Shader& shader = m_resource_manager.get<Shader>(handle);

		safe_release_com(shader.native.shader);
            
        m_resource_manager.deallocate<Shader>(handle);
    }

    Surface& RenderContext::get_surface(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<Surface>(handle);
    }

    Buffer& RenderContext::get_buffer(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<Buffer>(handle);
    }

    VertexBuffer& RenderContext::get_vertex_buffer(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<VertexBuffer>(handle);
    }

    IndexBuffer& RenderContext::get_index_buffer(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<IndexBuffer>(handle);
    }

    ConstantBuffer& RenderContext::get_constant_buffer(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<ConstantBuffer>(handle);
    }

    BlendState& RenderContext::get_blend_state(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<BlendState>(handle);
    }

    RasterizerState& RenderContext::get_rasterizer_state(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<RasterizerState>(handle);
    }

    InputSignature& RenderContext::get_input_signature(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<InputSignature>(handle);
    }

    Sampler& RenderContext::get_sampler(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<Sampler>(handle);
    }

    DepthStencilState& RenderContext::get_depth_stencil_state(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<DepthStencilState>(handle);
    }

    Shader& RenderContext::get_shader(HResource handle)
    {
        camy_assert(handle != kInvalidHResource);
        return m_resource_manager.get<Shader>(handle);
    }
}

#endif