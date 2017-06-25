/* command_list.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/command_list.hpp>

#if defined(CAMY_OS_WINDOWS) && defined(CAMY_BACKEND_D3D11)

// camy
#include <camy/render_context.hpp>

// D3D11
#include <d3d11_1.h>
#undef near
#undef far

namespace camy
{
    CommandList::CommandList() {}

    CommandList::~CommandList() {}

    void CommandList::begin()
    {
        RenderContextData& data = API::rc().get_platform_data();
        ContextID ctx_id = API::rc().id_for_current();
        m_data.ctx = data.contexts[ctx_id].deferred_ctx;

        if (m_data.command_list != nullptr)
        {
            m_data.command_list->Release();
            m_data.command_list = nullptr;
        }

        m_updates.clear();
    }

    void CommandList::end() 
	{ 
		if (m_data.command_list != nullptr)
			m_data.command_list->Release();
		m_data.ctx->FinishCommandList(false, &m_data.command_list); 
	}

#define _camy_cl_assert                                                                            \
    {                                                                                              \
        CAMY_ASSERT(m_data.ctx != nullptr);                                                        \
    }

    void CommandList::clear_color(HResource handle, const float4& color)
    {
        _camy_cl_assert;

        Surface& surface = API::rc().get_surface(handle);
        m_data.ctx->ClearRenderTargetView(surface.native.rtvs[0], (float*)&color);
    }

    void CommandList::clear_depth(HResource handle, float depth)
    {
        _camy_cl_assert;

        Surface& surface = API::rc().get_surface(handle);
        m_data.ctx->ClearDepthStencilView(surface.native.dsvs[0], D3D11_CLEAR_DEPTH, depth, 0);
    }

    void CommandList::clear_stencil(HResource handle, uint32 val)
    {
        _camy_cl_assert;

        Surface& surface = API::rc().get_surface(handle);
        m_data.ctx->ClearDepthStencilView(surface.native.dsvs[0], D3D11_CLEAR_STENCIL, 0.f, val);
    }

    void CommandList::set_vertex_shader(HResource handle)
    {
        _camy_cl_assert;

        ID3D11VertexShader* shader = nullptr;
        if (handle.is_valid())
            shader = (ID3D11VertexShader*)API::rc().get_shader(handle).native.shader;

        m_data.ctx->VSSetShader(shader, nullptr, 0);
    }

    void CommandList::set_geometry_shader(HResource handle)
    {
        _camy_cl_assert;

        ID3D11GeometryShader* shader = nullptr;
        if (handle.is_valid())
            shader = (ID3D11GeometryShader*)API::rc().get_shader(handle).native.shader;

        m_data.ctx->GSSetShader(shader, nullptr, 0);
    }

    void CommandList::set_pixel_shader(HResource handle)
    {
        _camy_cl_assert;

        ID3D11PixelShader* shader = nullptr;
        if (handle.is_valid())
            shader = (ID3D11PixelShader*)API::rc().get_shader(handle).native.shader;

        m_data.ctx->PSSetShader(shader, nullptr, 0);
    }

    void CommandList::set_targets(const HResource* render_targets,
                                  rsize num_render_targets,
                                  HResource depth_buffer,
                                  uint8* views)
    {
        uint8 default_views[API::MAX_RENDER_TARGETS] = {0, 0};
        if (views == nullptr) views = default_views;

        _camy_cl_assert;

        ID3D11DepthStencilView* dsv = nullptr;
        if (depth_buffer.is_valid()) dsv = API::rc().get_surface(depth_buffer).native.dsvs[0];

        ID3D11RenderTargetView* rtvs[API::MAX_RENDER_TARGETS]{nullptr, nullptr};
        for (rsize i = 0; i < num_render_targets; ++i)
        {
            if (render_targets[i].is_valid())
                rtvs[i] = API::rc().get_surface(render_targets[i]).native.rtvs[views[i]];
        }

        m_data.ctx->OMSetRenderTargets(num_render_targets, rtvs, dsv);
    }

    D3D11_PRIMITIVE_TOPOLOGY camy_to_d3d11(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList:
            return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:
            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:
            return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            CAMY_ASSERT(false);
			return (D3D11_PRIMITIVE_TOPOLOGY)-1;
        }
    }

    void CommandList::set_primitive_topology(PrimitiveTopology topology)
    {
        _camy_cl_assert;

        m_data.ctx->IASetPrimitiveTopology(camy_to_d3d11(topology));
    }

    void CommandList::set_rasterizer_state(HResource handle)
    {
        _camy_cl_assert;

        ID3D11RasterizerState* rss = nullptr;

        if (handle.is_valid()) rss = API::rc().get_rasterizer_state(handle).native.state;

        m_data.ctx->RSSetState(rss);
    }

    void CommandList::set_depth_stencil_state(HResource handle)
    {
        _camy_cl_assert;

        ID3D11DepthStencilState* dss = nullptr;

        if (handle.is_valid()) dss = API::rc().get_depth_stencil_state(handle).native.state;

        m_data.ctx->OMSetDepthStencilState(dss, 1); // TODO: Add stencil
    }

    void CommandList::set_input_signature(HResource handle)
    {
        _camy_cl_assert;

        ID3D11InputLayout* layout = nullptr;

        if (handle.is_valid()) layout = API::rc().get_input_signature(handle).native.input_layout;

        m_data.ctx->IASetInputLayout(layout);
    }

    void CommandList::set_viewport(const Viewport& viewport)
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
        CAMY_ASSERT(slot < API::MAX_VERTEX_BUFFERS);
        _camy_cl_assert;

        ID3D11Buffer* buffer = nullptr;
        UINT stride = 0;
        if (handle.is_valid())
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
        CAMY_ASSERT(m_data.ctx != nullptr);
        CAMY_ASSERT(slot + num_handles <= API::MAX_VERTEX_BUFFERS);

        ID3D11Buffer* vbs[API::MAX_VERTEX_BUFFERS];
        UINT strides[API::MAX_VERTEX_BUFFERS];
        UINT offsets[API::MAX_VERTEX_BUFFERS];
        for (rsize i = 0; i < num_handles; ++i)
        {
            if (handles[i].is_valid())
            {
                VertexBuffer& vertex_buffer = API::rc().get_vertex_buffer(handles[i]);
                vbs[i] = vertex_buffer.native.buffer;
                strides[i] = vertex_buffer.native.stride;
            }
            else
            {
                vbs[i] = nullptr;
                strides[i] = 0;
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
        if (handle.is_valid())
        {
            IndexBuffer& ib = API::rc().get_index_buffer(handle);
            buffer = ib.native.buffer;
            format = (DXGI_FORMAT)ib.native.dxgi_format;
        }

        m_data.ctx->IASetIndexBuffer(buffer, format, 0);
    }

    void CommandList::set_cbuffer(ShaderVariable var, HResource handle)
    {
        _camy_cl_assert;

        ConstantBuffer& cbuffer = API::rc().get_constant_buffer(handle);

        switch ((ShaderDesc::Type)var.shader())
        {
        case ShaderDesc::Type::Vertex:
            m_data.ctx->VSSetConstantBuffers(var.slot(), 1, &cbuffer.native.buffer);
        case ShaderDesc::Type::Pixel:
            m_data.ctx->PSSetConstantBuffers(var.slot(), 1, &cbuffer.native.buffer);
        case ShaderDesc::Type::Geometry:
            m_data.ctx->GSSetConstantBuffers(var.slot(), 1, &cbuffer.native.buffer);
		default:
			CAMY_ASSERT(false);
        }
    }

    void CommandList::set_cbuffer_off(ShaderVariable var, HResource handle, rsize offset)
    {
        _camy_cl_assert;

        ConstantBuffer& cbuffer = API::rc().get_constant_buffer(handle);
        CAMY_ASSERT(cbuffer.desc.size % 16 == 0);
        rsize cbuffer_num_constants = var.size() / 16;

        if (cbuffer_num_constants % 16 != 0)
        {
            CL_ERR("Constant Buffers must be multiples of 256 bytes in size that is multiples of "
                   "16 constants (4 * 32-bit)");
            CL_ERR("> The provided one has: ", cbuffer_num_constants, " constants");
            return;
        }

        switch ((ShaderDesc::Type)var.shader())
        {
        case ShaderDesc::Type::Vertex:
            m_data.ctx->VSSetConstantBuffers1(var.slot(), 1, &cbuffer.native.buffer, &offset,
                                              &cbuffer_num_constants);
            break;
        case ShaderDesc::Type::Geometry:
            m_data.ctx->GSSetConstantBuffers1(var.slot(), 1, &cbuffer.native.buffer, &offset,
                                              &cbuffer_num_constants);
            break;
        case ShaderDesc::Type::Pixel:
            m_data.ctx->PSSetConstantBuffers1(var.slot(), 1, &cbuffer.native.buffer, &offset,
                                              &cbuffer_num_constants);
            break;
        }
    }

    void
    _set_sampler_vs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->VSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void
    _set_surface_vs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->VSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[view]);
    }

    void _set_buffer_vs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->VSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void
    _set_sampler_gs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->GSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void
    _set_surface_gs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->GSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[view]);
    }

    void _set_buffer_gs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->GSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void
    _set_sampler_ps(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->PSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void
    _set_surface_ps(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->PSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[view]);
    }

    void _set_buffer_ps(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->PSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void
    _set_sampler_cs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->CSSetSamplers(var.slot(), 1, &API::rc().get_sampler(handle).native.sampler);
    }

    void
    _set_surface_cs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->CSSetShaderResources(var.slot(), 1, &API::rc().get_surface(handle).native.srvs[view]);
    }

    void _set_buffer_cs(ID3D11DeviceContext1* ctx, ShaderVariable var, HResource handle, uint8 view)
    {
        ctx->CSSetShaderResources(var.slot(), 1, &API::rc().get_buffer(handle).native.srv);
    }

    void (*_set_ftable[])(ID3D11DeviceContext1*, ShaderVariable, HResource, uint8){
        _set_sampler_vs, _set_surface_vs, _set_buffer_vs,

        _set_sampler_gs, _set_surface_gs, _set_buffer_gs,

        _set_sampler_ps, _set_surface_ps, _set_buffer_ps,

        _set_sampler_cs, _set_surface_cs, _set_buffer_cs};

    void CommandList::set_parameter(ShaderVariable var, HResource handle, uint8 view)
    {
        _camy_cl_assert;

        rsize lookup_idx = (rsize)var.shader() * 3 + var.type();
        _set_ftable[lookup_idx](m_data.ctx, var, handle, view);
    }

    void CommandList::draw(uint32 vertex_count, uint32 vertex_offset)
    {
        CAMY_ASSERT(m_data.ctx != nullptr);
        m_data.ctx->Draw(vertex_count, vertex_offset);
    }

    void CommandList::draw_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset)
    {
        CAMY_ASSERT(m_data.ctx != nullptr);
        m_data.ctx->DrawIndexed(index_count, index_offset, vertex_offset);
    }

    void CommandList::draw_instanced(uint32 vertex_count,
                                     uint32 instance_count,
                                     uint32 vertex_offset,
                                     uint32 instance_offset)
    {
        CAMY_ASSERT(m_data.ctx != nullptr);
        m_data.ctx->DrawInstanced(vertex_count, instance_count, vertex_offset, instance_offset);
    }

    void CommandList::draw_indexed_instanced(uint32 index_count,
                                             uint32 instance_count,
                                             uint32 index_offset,
                                             uint32 vertex_offset,
                                             uint32 instance_offset)
    {
        CAMY_ASSERT(m_data.ctx != nullptr);
        m_data.ctx->DrawIndexedInstanced(index_count, instance_count, index_offset, vertex_offset,
                                         instance_offset);
    }

    void CommandList::queue_constant_buffer_update(HResource handle, const void* data, rsize bytes)
    {
        _camy_cl_assert;
        CAMY_ASSERT(data != nullptr);
        CAMY_ASSERT(bytes != 0 && bytes % 16 == 0);

        m_updates.append({data, bytes, handle});
    }
}

#endif