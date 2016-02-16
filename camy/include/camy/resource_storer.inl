namespace camy
{
	/* Allocate */
	template <>
	inline Surface* ResourceStorer::allocate()
	{
		return m_surfaces.allocate();
	}

	template <>
	inline Buffer* ResourceStorer::allocate()
	{
		return m_buffers.allocate();
	}

	template <>
	inline VertexBuffer* ResourceStorer::allocate()
	{
		return m_vertex_buffers.allocate();
	}

	template <>
	inline IndexBuffer* ResourceStorer::allocate()
	{
		return m_index_buffers.allocate();
	}

	template <>
	inline ConstantBuffer* ResourceStorer::allocate()
	{
		return m_constant_buffers.allocate();
	}

	template <>
	inline BlendState* ResourceStorer::allocate()
	{
		return m_blend_states.allocate();
	}

	template <>
	inline RasterizerState* ResourceStorer::allocate()
	{
		return m_rasterizer_states.allocate();
	}

	template <>
	inline InputSignature* ResourceStorer::allocate()
	{
		return m_input_signatures.allocate();
	}

	template <>
	inline Sampler* ResourceStorer::allocate()
	{
		return m_samplers.allocate();
	}

	template <>
	inline hidden::Shader* ResourceStorer::allocate()
	{
		return m_shaders.allocate();
	}

	template <>
	inline DepthStencilState* ResourceStorer::allocate()
	{
		return m_depth_stencil_states.allocate();
	}

	/* Deallocate */
	template <>
	inline void ResourceStorer::deallocate(Surface* ptr)
	{
		if (ptr != nullptr)
			m_surfaces.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(Buffer* ptr)
	{
		if (ptr != nullptr)
			m_buffers.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(VertexBuffer* ptr)
	{
		if (ptr != nullptr)
			m_vertex_buffers.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(IndexBuffer* ptr)
	{
		if (ptr != nullptr)
			m_index_buffers.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(ConstantBuffer* ptr)
	{
		if (ptr != nullptr)
			m_constant_buffers.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(BlendState* ptr)
	{
		if (ptr != nullptr)
			m_blend_states.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(RasterizerState* ptr)
	{
		if (ptr != nullptr)
			m_rasterizer_states.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(InputSignature* ptr)
	{
		if (ptr != nullptr)
			m_input_signatures.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(Sampler* ptr)
	{
		if (ptr != nullptr)
			m_samplers.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(hidden::Shader* ptr)
	{
		if (ptr != nullptr)
			m_shaders.deallocate(ptr);
	}

	template <>
	inline void ResourceStorer::deallocate(DepthStencilState* ptr)
	{
		if (ptr != nullptr)
			m_depth_stencil_states.deallocate(ptr);
	}
}