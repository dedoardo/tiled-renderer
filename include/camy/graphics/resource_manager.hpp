/* resource_manager.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/memory/alloc.hpp>
#include <camy/core/memory/pool.hpp>
#include <camy/core/memory/static_string.hpp>
#include <camy/graphics/base.hpp>

namespace camy
{
#if defined(camy_enable_memory_tracking)
    struct AllocationInfo
    {
        StaticString<19> name;
        uint16           line;
        const char8*     file;
    };
#else
    struct AllocationInfo { };
#endif  
    template <typename ResourceType>
    struct TrackableResource
    {
        AllocationInfo info;
        ResourceType   resource;
    };

    using TrackableSurface = TrackableResource<Surface>;
    using TrackableBuffer = TrackableResource<Buffer>;
    using TrackableVertexBuffer = TrackableResource<VertexBuffer>;
    using TrackableIndexBuffer = TrackableResource<IndexBuffer>;
    using TrackableConstantBuffer = TrackableResource<ConstantBuffer>;
    using TrackableBlendState = TrackableResource<BlendState>;
    using TrackableRasterizerState = TrackableResource<RasterizerState>;
    using TrackableShader = TrackableResource<Shader>;
    using TrackableInputSignature = TrackableResource<InputSignature>;
    using TrackableSampler = TrackableResource<Sampler>;
    using TrackableDepthStencilState = TrackableResource<DepthStencilState>;

#pragma pack(push, 1)
	//! Handle to a resource, wrapped in a struct w/ cast and non-explicit constructur
	//! in order to provide a similar interface to ShaderVariable.
	struct camy_api HResource
	{
		uint16 _v = (uint16)-1;

		HResource(uint16 val = -1) { _v = val; }
		operator uint16() { return _v; }
		bool is_valid()const { return _v != (uint16)-1; }
		bool is_invalid()const { return _v == (uint16)-1; }
		
		constexpr static uint16 make_invalid() { return (uint16)-1; }
	};
#pragma pack(pop)

    class camy_api ResourceManager final
    {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        template <typename ResourceType>
        HResource allocate(const char8* file, uint16 line, const char8* name = nullptr);

        template <typename ResourceType>
        void deallocate(HResource handle);

        template <typename ResourceType>
        ResourceType& get(HResource handle);        
    private:
        Pool<TrackableSurface>           m_surfaces;
        Pool<TrackableBuffer>            m_buffers;
        Pool<TrackableVertexBuffer>      m_vertex_buffers;
        Pool<TrackableIndexBuffer>       m_index_buffers;
        Pool<TrackableConstantBuffer>    m_constant_buffers;
        Pool<TrackableBlendState>        m_blend_states;
        Pool<TrackableRasterizerState>   m_rasterizer_states;
        Pool<TrackableShader>            m_shaders;
        Pool<TrackableInputSignature>    m_input_signatures;
        Pool<TrackableSampler>           m_samplers;
        Pool<TrackableDepthStencilState> m_depth_stencil_states;
    };

#define _camy_rm_allocate(type, pool) type res;  res.info.file = file; res.info.line = line; res.info.name = name; return (HResource)pool.reserve();
    template <>
    inline HResource ResourceManager::allocate<Surface>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableSurface, m_surfaces) }
    template <>
    inline HResource ResourceManager::allocate<Buffer>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableBuffer, m_buffers) }
    template <>
    inline HResource ResourceManager::allocate<VertexBuffer>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableVertexBuffer, m_vertex_buffers) }
    template <>
    inline HResource ResourceManager::allocate<IndexBuffer>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableIndexBuffer, m_index_buffers) }
    template <>
    inline HResource ResourceManager::allocate<ConstantBuffer>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableConstantBuffer, m_constant_buffers) }
    template <>
    inline HResource ResourceManager::allocate<BlendState>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableBlendState, m_blend_states) }
    template <>
    inline HResource ResourceManager::allocate<RasterizerState>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableRasterizerState, m_rasterizer_states) }
    template <>
    inline HResource ResourceManager::allocate<Shader>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableShader, m_shaders) }
    template <>
    inline HResource ResourceManager::allocate<InputSignature>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableInputSignature, m_input_signatures) }
    template <>
    inline HResource ResourceManager::allocate<Sampler>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableSampler, m_samplers) }
    template <>
    inline HResource ResourceManager::allocate<DepthStencilState>(const char8* file, uint16 line, const char8* name) { _camy_rm_allocate(TrackableDepthStencilState, m_depth_stencil_states) }

    template <>
    inline void ResourceManager::deallocate<Surface>(HResource handle) { m_surfaces.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<Buffer>(HResource handle) { m_buffers.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<VertexBuffer>(HResource handle) { m_vertex_buffers.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<IndexBuffer>(HResource handle) { m_index_buffers.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<ConstantBuffer>(HResource handle) { m_constant_buffers.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<BlendState>(HResource handle) { m_blend_states.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<RasterizerState>(HResource handle) { m_rasterizer_states.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<Shader>(HResource handle) { m_shaders.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<InputSignature>(HResource handle) { m_input_signatures.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<Sampler>(HResource handle) { m_samplers.deallocate((rsize)handle); }
    template <>
    inline void ResourceManager::deallocate<DepthStencilState>(HResource handle) { m_depth_stencil_states.deallocate((rsize)handle); }

    template <>
    inline Surface& ResourceManager::get(HResource handle) { return m_surfaces.get((rsize)handle).resource; }
    template <>
    inline Buffer& ResourceManager::get(HResource handle) { return m_buffers.get((rsize)handle).resource; }
    template <>
    inline VertexBuffer& ResourceManager::get(HResource handle) { return m_vertex_buffers.get((rsize)handle).resource; }
    template <>
    inline IndexBuffer& ResourceManager::get(HResource handle) { return m_index_buffers.get((rsize)handle).resource; }
    template <>
    inline ConstantBuffer& ResourceManager::get(HResource handle) { return  m_constant_buffers.get((rsize)handle).resource; }
    template <>
    inline BlendState& ResourceManager::get(HResource handle) { return m_blend_states.get((rsize)handle).resource; }
    template <>
    inline RasterizerState& ResourceManager::get(HResource handle) { return m_rasterizer_states.get((rsize)handle).resource; }
    template <>
    inline Shader& ResourceManager::get(HResource handle) { return m_shaders.get((rsize)handle).resource; }
    template <>
    inline InputSignature& ResourceManager::get(HResource handle) { return m_input_signatures.get((rsize)handle).resource; }
    template <>
    inline Sampler& ResourceManager::get(HResource handle) { return m_samplers.get((rsize)handle).resource; }
    template <>
    inline DepthStencilState& ResourceManager::get(HResource handle) { return m_depth_stencil_states.get((rsize)handle).resource; }
}