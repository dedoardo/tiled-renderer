namespace camy
{
	template <>
	inline HResource ResourceManager::allocate<Surface>() { return (HResource)m_surfaces.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<Buffer>() { return (HResource)m_buffers.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<VertexBuffer>() { return (HResource)m_vertex_buffers.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<IndexBuffer>() { return (HResource)m_index_buffers.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<ConstantBuffer>() { return (HResource)m_constant_buffers.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<BlendState>() { return (HResource)m_blend_states.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<RasterizerState>() { return (HResource)m_rasterizer_states.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<Shader>() { return (HResource)m_shaders.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<InputSignature>() { return (HResource)m_input_signatures.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<Sampler>() { return (HResource)m_samplers.reserve(); }
	template <>
	inline HResource ResourceManager::allocate<DepthStencilState>() { return (HResource)m_depth_stencil_states.reserve(); }

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
	inline Surface& ResourceManager::get(HResource handle) { return m_surfaces.get((rsize)handle); }
	template <>
	inline Buffer& ResourceManager::get(HResource handle) { return m_buffers.get((rsize)handle); }
	template <>
	inline VertexBuffer& ResourceManager::get(HResource handle) { return m_vertex_buffers.get((rsize)handle); }
	template <>
	inline IndexBuffer& ResourceManager::get(HResource handle) { return m_index_buffers.get((rsize)handle); }
	template <>
	inline ConstantBuffer& ResourceManager::get(HResource handle) { return  m_constant_buffers.get((rsize)handle); }
	template <>
	inline BlendState& ResourceManager::get(HResource handle) { return m_blend_states.get((rsize)handle); }
	template <>
	inline RasterizerState& ResourceManager::get(HResource handle) { return m_rasterizer_states.get((rsize)handle); }
	template <>
	inline Shader& ResourceManager::get(HResource handle) { return m_shaders.get((rsize)handle); }
	template <>
	inline InputSignature& ResourceManager::get(HResource handle) { return m_input_signatures.get((rsize)handle); }
	template <>
	inline Sampler& ResourceManager::get(HResource handle) { return m_samplers.get((rsize)handle); }
	template <>
	inline DepthStencilState& ResourceManager::get(HResource handle) { return m_depth_stencil_states.get((rsize)handle); }

}