#pragma once

// camy
#include "resources.hpp"
#include "shader.hpp"
#include "common_structs.hpp"

namespace camy
{
	enum PipelineStates
	{
		PipelineStates_None = 0,
		PipelineStates_CommonStates = 1 << 0,
		PipelineStates_VertexShader = 1 << 1,
		PipelineStates_GeometryShader = 1 << 2,
		PipelineStates_PixelShader = 1 << 3,
		PipelineStates_VertexBuffer1 = 1 << 4,
		PipelineStates_VertexBuffer2 = 1 << 5,
		PipelineStates_IndexBuffer = 1 << 6,
		PipelineStates_PrimitiveTopology = 1 << 7,
	};

	/*
		As described in GPUBackend::execute() this is the current method, when working
		with stategroups the whole caching system would be slightly different.
	*/
	struct PipelineCache
	{
		PipelineCache();
		
		// Common pipeline states
		CommonStates* common_states{ nullptr };

		// Cache for shaders
		const ParameterGroup*   parameter_cache[features::num_cache_slots];
		CachedConstantBuffer*	cbuffer_cache[features::max_bindable_constant_buffers];

		// Common states
		VertexBuffer*		vertex_buffer1{ nullptr };
		VertexBuffer*		vertex_buffer2{ nullptr };
		IndexBuffer*		index_buffer{ nullptr };
		PrimitiveTopology	primitive_topology{ PrimitiveTopology::TriangleList };
		Shader*				shaders[Shader::num_types];

		// Used to check whether nullptrs refer to actual null states or not yet set
		u32 states_set{ PipelineStates_None };
	};

	inline PipelineCache::PipelineCache()
	{
		// Setting arrays to null
		std::memset(parameter_cache, 0, sizeof(ParameterGroup*) * features::num_cache_slots);
		std::memset(cbuffer_cache, 0, sizeof(CachedConstantBuffer*) * features::max_bindable_constant_buffers);
		std::memset(shaders, 0, sizeof(Shader*) * Shader::num_types);
	}
}