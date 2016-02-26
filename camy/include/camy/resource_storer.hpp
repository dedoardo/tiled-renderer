#pragma once

// camy
#include "base.hpp"
#include "allocators/paged_pool_allocator.hpp"
#include "resources.hpp"

namespace camy
{
	class ResourceStorer final
	{
	public:
		ResourceStorer() = default;
		~ResourceStorer() = default;

		template <typename ResourceType>
		ResourceType* allocate();

		template <typename ResourceType>
		void deallocate(ResourceType* resource);

	private:

		allocators::PagedPoolAllocator<Surface>			m_surfaces;
		allocators::PagedPoolAllocator<Buffer>			m_buffers;
		allocators::PagedPoolAllocator<VertexBuffer>	m_vertex_buffers;
		allocators::PagedPoolAllocator<IndexBuffer>		m_index_buffers;
		allocators::PagedPoolAllocator<ConstantBuffer>  m_constant_buffers;
		allocators::PagedPoolAllocator<BlendState>		m_blend_states;
		allocators::PagedPoolAllocator<RasterizerState> m_rasterizer_states;
		allocators::PagedPoolAllocator<InputSignature>  m_input_signatures;
		allocators::PagedPoolAllocator<Sampler>			m_samplers;
		allocators::PagedPoolAllocator<hidden::Shader>	m_shaders;
		allocators::PagedPoolAllocator<DepthStencilState> m_depth_stencil_states;
	};
}

#include "resource_storer.inl"