/* command_list.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/command_list.hpp>

#if defined(camy_os_windows) && defined(camy_backend_d3d11)

// camy
#include <camy/graphics/platform.hpp>
#include <camy/graphics/render_context.hpp>

// D3D11
#include <d3d11_1.h>
#undef near
#undef far

namespace camy
{
    CommandList::CommandList()
    {

    }

    CommandList::~CommandList()
    {

    }

    void CommandList::begin()
    {
        RenderContextData& data = API::rc().get_platform_data();
        ContextID ctx_id = API::rc().get_id_for_current();
        m_data.ctx = data.contexts[0].deferred_ctx;

        if (m_data.command_list != nullptr)
        {
            m_data.command_list->Release();
            m_data.command_list = nullptr;
        }

        m_updates.clear();
    }

    void CommandList::end()
    {
        m_data.ctx->FinishCommandList(false, &m_data.command_list);
    }

#define _camy_cl_assert { camy_assert(m_data.ctx != nullptr); }

    void CommandList::clear_color(HResource handle, const float4& color)
    {
        _camy_cl_assert;
        
        Surface& surface = API::rc().get_surface(handle);
        m_data.ctx->ClearRenderTargetView(surface.native.rtvs[0], (float*)&color);
    }
    
    void CommandList::clear_depth_stencil(HResource handle, float depth, uint stencil)
    {
        _camy_cl_assert;

        Surface& surface = API::rc().get_surface(handle);

        m_data.ctx->ClearDepthStencilView(surface.native.dsvs[0], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, (UINT8)stencil);
    }

    void CommandList::set_vertex_shader(HResource handle)
    {
        _camy_cl_assert;

        ID3D11VertexShader* shader = nullptr;
        if (handle != kInvalidHResource)
            shader = (ID3D11VertexShader*)API::rc().get_shader(handle).native.shader;

        m_data.ctx->VSSetShader(shader, nullptr, 0);
    }

    void CommandList::set_geometry_shader(HResource handle)
    {
        _camy_cl_assert;
    
        ID3D11GeometryShader* shader = nullptr;
        if (handle != kInvalidHResource)
            shader = (ID3D11GeometryShader*)API::rc().get_shader(handle).native.shader;

        m_data.ctx->GSSetShader(shader, nullptr, 0);
    }

    void CommandList::set_pixel_shader(HResource handle)
    {
        _camy_cl_assert;

        ID3D11PixelShader* shader = nullptr;
        if (handle != kInvalidHResource)
            shader = (ID3D11PixelShader*)API::rc().get_shader(handle).native.shader;

        m_data.ctx->PSSetShader(shader, nullptr, 0);
    }

    void CommandList::set_targets(const HResource* render_targets, rsize num_render_targets, HResource depth_buffer)
    {
        _camy_cl_assert;

        ID3D11DepthStencilView* dsv = nullptr;
        if (depth_buffer != kInvalidHResource)
            dsv = API::rc().get_surface(depth_buffer).native.dsvs[0];

        ID3D11RenderTargetView* rtvs[kMaxBindableRenderTargets]{ nullptr, nullptr };
        for (rsize i = 0; i < num_render_targets; ++i)
        {
            if (render_targets[i] != kInvalidHResource)
                rtvs[i] = API::rc().get_surface(render_targets[i]).native.rtvs[0];
        }

        m_data.ctx->OMSetRenderTargets(num_render_targets, rtvs, dsv);
    }

    void CommandList::set_primitive_topology()
    {
        _camy_cl_assert;

        m_data.ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

	void CommandList::set_rasterizer_state(HResource handle)
	{
		_camy_cl_assert;

		ID3D11RasterizerState* rss = nullptr;

		if (handle != kInvalidHResource)
			rss = API::rc().get_rasterizer_state(handle).native.state;

		m_data.ctx->RSSetState(rss);
	}

    void CommandList::set_input_signature(HResource handle)
    {
        _camy_cl_assert;

        ID3D11InputLayout* layout = nullptr;

        if (handle != kInvalidHResource)
            layout = API::rc().get_input_signature(handle).native.input_layout;

        m_data.ctx->IASetInputLayout(layout);
    }

    void CommandList::set_viewport(const Viewport & viewport)
    {
        _camy_cl_assert;

        D3D11_VIEWPORT d3d11_viewport;
        d3d11_viewport.MinDepth = viewport.near;
        d3d11_viewport.MaxDepth = viewport.far;
        d3d11_viewport.TopLeftX = (float)viewport.left;
        d3d11_viewport.TopLeftY = (float)viewport.top;
        d3d11_viewport.Width = (float)(viewport.right - viewport.left);
        d3d11_viewport.Height = (float)(viewport.bottom - viewport.top);

        m_data.ctx->RSSetViewports(1, &d3d11_viewport);
    }

    void CommandList::set_vertex_buffer(rsize slot, HResource handle)
    {
        // slots are 0-indexed
        camy_assert(slot < kMaxBindableVertexBuffers);
        _camy_cl_assert;

        ID3D11Buffer* buffer = nullptr;
        UINT stride = 0;
        if (handle != kInvalidHResource)
        {
            VertexBuffer& vb = API::rc().get_vertex_buffer(handle);
            buffer = vb.native.buffer;
            stride = vb.native.stride;
        }
        UINT offset = 0;
        m_data.ctx->IASetVertexBuffers(slot, 1, &buffer, &stride, &offset);
    }

    void CommandList::set_vertex_buffers(rsize slot, const HResource* handles, rsize num_handles)
    {
        camy_assert(m_data.ctx != nullptr);
        camy_assert(slot + num_handles <= kMaxBindableVertexBuffers);
        
        ID3D11Buffer* vbs[kMaxBindableVertexBuffers];
        UINT strides[kMaxBindableVertexBuffers];
        UINT offsets[kMaxBindableVertexBuffers];
        for (rsize i = 0; i < num_handles; ++i)
        {
            if (handles[i] != kInvalidHResource)
            {
                VertexBuffer& vertex_buffer = API::rc().get_vertex_buffer(handles[i]);
                vbs[i] = vertex_buffer.native.buffer;
                strides[i] = vertex_buffer.native.stride;
            }
            else
            {
                vbs[i] = nullptr; strides[i] = 0;
            }
            offsets[i] = 0;
        }

        m_data.ctx->IASetVertexBuffers(slot, num_handles, vbs, strides, offsets);
    }

    void CommandList::set_index_buffer(HResource handle)
    {
        _camy_cl_assert;

        ID3D11Buffer* buffer = nullptr;
        DXGI_FORMAT format = DXGI_FORMAT_R16_UINT;
        if (handle != kInvalidHResource)
        {
            IndexBuffer& ib = API::rc().get_index_buffer(handle);
            buffer = ib.native.buffer;
            format = (DXGI_FORMAT)ib.native.dxgi_format;
        }

        m_data.ctx->IASetIndexBuffer(buffer, format, 0);
    }

    void CommandList::set_parameter(ShaderVariable var, const void* data, HResource handle, rsize offset)
    {
        _camy_cl_assert;
        camy_assert(data != nullptr);

        ConstantBuffer& cbuffer = API::rc().get_constant_buffer(handle);
        camy_assert(cbuffer.desc.size % 16 == 0);
        rsize cbuffer_num_constants = var.size() / 16;

        switch ((ShaderDesc::Type)var.shader())
        {
        case ShaderDesc::Type::Vertex:
            m_data.ctx->VSSetConstantBuffers1(var.slot(), 1, &cbuffer.native.buffer, &offset, &cbuffer_num_constants);
            break;
        case ShaderDesc::Type::Geometry:
            m_data.ctx->GSSetConstantBuffers1(var.slot(), 1, &cbuffer.native.buffer, &offset, &cbuffer_num_constants);
            break;
        case ShaderDesc::Type::Pixel:
            m_data.ctx->PSSetConstantBuffers1(var.slot(), 1, &cbuffer.native.buffer, &offset, &cbuffer_num_constants);
            break;
        }
    }

    void _set_sampler_vs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->VSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void _set_surface_vs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->VSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[0]);
    }

    void _set_buffer_vs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->VSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void _set_sampler_gs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->GSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void _set_surface_gs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->GSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[0]);
    }

    void _set_buffer_gs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->GSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void _set_sampler_ps(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->PSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void _set_surface_ps(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->PSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[0]);
    }

    void _set_buffer_ps(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->PSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void _set_sampler_cs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->CSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void _set_surface_cs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->CSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[0]);
    }

    void _set_buffer_cs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle)
    {
        ctx->CSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void (*_set_ftable[])(ID3D11DeviceContext1*, ShaderVariable, HResource)
    {
        _set_sampler_vs,
        _set_surface_vs,
        _set_buffer_vs,

        _set_sampler_gs,
        _set_surface_gs,
        _set_buffer_gs,

        _set_sampler_ps,
        _set_surface_ps,
        _set_buffer_ps, 
            
        _set_sampler_cs,
        _set_surface_cs,
        _set_buffer_cs
    };

    void CommandList::set_parameter(ShaderVariable var, HResource handle)
    {
        _camy_cl_assert;

        rsize lookup_idx = (rsize)var.shader() * (rsize)ShaderDesc::Type::Count + var.type();
        _set_ftable[lookup_idx](m_data.ctx, var, handle);
    }

    void CommandList::draw(uint32 vertex_count, uint32 vertex_offset)
    {
        camy_assert(m_data.ctx != nullptr);
        m_data.ctx->Draw(vertex_count, vertex_offset);
    }

    void CommandList::draw_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset)
    {
        camy_assert(m_data.ctx != nullptr);
        m_data.ctx->DrawIndexed(index_count, index_offset, vertex_offset);
    }

    void CommandList::draw_instanced(uint32 vertex_count, uint32 instance_count, uint32 vertex_offset, uint32 instance_offset)
    {
        camy_assert(m_data.ctx != nullptr);
        m_data.ctx->DrawInstanced(vertex_count, instance_count, vertex_offset, instance_offset);
    }

    void CommandList::draw_indexed_instanced(uint32 index_count, uint32 instance_count, uint32 index_offset, uint32 vertex_offset, uint32 instance_offset)
    {
        camy_assert(m_data.ctx != nullptr);
        m_data.ctx->DrawIndexedInstanced(index_count, instance_count, index_offset, vertex_offset, instance_offset);
    }

    void CommandList::queue_constant_buffer_update(HResource handle, const void* data, rsize bytes)
    {
        _camy_cl_assert;
        camy_assert(data != nullptr);
        camy_assert(bytes != 0 && bytes % 16 == 0);

        m_updates.append({ data, bytes, handle });
    }
}

#endif