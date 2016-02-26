// Header
#include <camy/cbuffer_system.hpp>

// camy
#include <camy/init.hpp>

namespace camy
{
	CBufferSystem::CBufferSystem() :
		m_hierarchy_depth{ 0 }
	{
		for (auto i{ 0u }; i < features::max_bindable_constant_buffers; ++i)
			m_cbuffers[i] = nullptr;
	}

	CBufferSystem::~CBufferSystem()
	{
		unload();
	}

	bool CBufferSystem::load()
	{
		unload();

		// Calculating hierarchy depth
		m_hierarchy_depth = 0;
		auto cur_size{ 16 };

		while (cur_size / 2 <= features::max_constant_buffer_size)
		{
			cur_size = cur_size << 1;
			++m_hierarchy_depth;
		}

		// Allocating matrix
		auto cbuffer_matrix{ new CachedConstantBuffer[m_hierarchy_depth * features::max_bindable_constant_buffers] };

		for (auto i{ 0u }; i < features::max_bindable_constant_buffers; ++i)
			m_cbuffers[i] = &cbuffer_matrix[i * m_hierarchy_depth]; // Assigning pointer to each row in the matrix

		// Creating constant buffers
		for (auto i{ 0u }; i < features::max_bindable_constant_buffers; ++i)
		{
			auto cur_size{ 16 };
			for (auto j{ 0u }; j < m_hierarchy_depth; ++j)
			{
				m_cbuffers[i][j].cbuffer = hidden::gpu.create_constant_buffer(cur_size);
				m_cbuffers[i][j].last_data = nullptr;

				if (m_cbuffers[i][j].cbuffer == nullptr)
				{
					camy_error("Failed to generate cbuffer hierarchy because CBuffer of size: ", cur_size, " failed to create");
					unload();
					return false;
				}

				cur_size *= 2;
			}
		}

		return true;
	}

	void CBufferSystem::unload()
	{
		safe_release_array(m_cbuffers[0]);
	}

	CachedConstantBuffer* CBufferSystem::get(u32 slot, u32 size)
	{
		camy_assert(size <= features::max_constant_buffer_size, { return nullptr; },
			"Requested CBuffer size is higher than supported: ", size);
		camy_assert(slot <= features::max_bindable_constant_buffers, { return nullptr; },
			"Requested slot is not valid: ", slot);

		// We start from 16, with size = 16  10000 => upper_pow2 returns 5 that for us is 0
		u32 index = math::upper_pow2(size) - 4;

		if (index >= m_hierarchy_depth)
		{
			camy_warning("Requested cbuffer resulted in invalid index computation: ", slot, " | ", size);
			return nullptr;
		}

		return &m_cbuffers[slot][index];
	}
}