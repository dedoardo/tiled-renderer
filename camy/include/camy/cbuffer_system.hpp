#pragma once

// camy
#include "camy_base.hpp"
#include "features.hpp"

namespace camy
{
	// Forward declaration
	class GPUBackend;
	struct ConstantBuffer;

	struct CachedConstantBuffer
	{
		ConstantBuffer* cbuffer{ nullptr };
		const void*			last_data{ nullptr };
	};

	class CBufferSystem 
	{
	public:
		CBufferSystem();
		~CBufferSystem();

		// Not intended use, just load() another one
		CBufferSystem(const CBufferSystem& other) = delete;
		CBufferSystem& operator=(const CBufferSystem& other) = delete;

		CBufferSystem(CBufferSystem&& other) = default;
		CBufferSystem& operator=(CBufferSystem&& other) = default;

		bool load();
		void unload();

		CachedConstantBuffer* get(u32 slot, u32 size);

	private:
		CachedConstantBuffer* m_cbuffers[features::max_bindable_constant_buffers];
		u32		m_hierarchy_depth;
	};
}