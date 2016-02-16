namespace camy
{
	template <typename ItemType>
	Queue<ItemType>::Queue() :
		m_shared_parameters{ nullptr },
		m_dependencies{ nullptr },
		m_num_dependencies{ 0 },
		m_is_ready{ false }
	{

	}

	template <typename ItemType>
	void Queue<ItemType>::begin()
	{
		// Clearing the queue
		m_render_items.clear();
		m_is_ready = false;

		m_shared_parameters = nullptr;
		m_dependencies = nullptr;
		m_num_dependencies = 0;
	}

	template <typename ItemType>
	ItemType* Queue<ItemType>::create_item()
	{
		m_render_items.emplace_back();
		return &m_render_items.back();
	}
	
	template <typename ItemType>
	void Queue<ItemType>::end(const ParameterGroup* shared_parameters, Dependency* dependencies, const u32 num_dependencies)
	{
		m_shared_parameters = shared_parameters;

		m_dependencies = dependencies;
		m_num_dependencies = num_dependencies;

		/*
		Temporal coherence :
		http://www.gamedev.net/topic/661114-temporal-coherence-and-render-queue-sorting/
		thanks L. Spiro
		*/
		auto current_size{ m_sorted_indices.size() };
		auto next_size{ m_render_items.size() };

		if (next_size > current_size)
		{
			// Adding last indices to the end of the queue
			for (auto i{ current_size }; i < next_size; ++i)
				m_sorted_indices.push_back(static_cast<u32>(i));
		}

		// We need to reset the indices
		if (next_size < current_size)
		{
			m_sorted_indices.resize(next_size); // Resetting to new size
			for (auto i{ 0u }; i < next_size; ++i)
				m_sorted_indices[i] = static_cast<u32>(i);
		}

		// If size is the same we don't do anything, time to sort.
		// We use insertion sort to make full use of probably already kind of sorted queue
		// When resetting the indices a classic qsort might be more efficient //? Todo : Verify last statement
		// Does it matter if i sort from left to right ? cache ? 
		for (auto i{ 1u }; i < next_size; ++i)
		{
			auto j{ i };
			ItemType::Key key_i{ m_render_items[m_sorted_indices[i]].key };

			ItemType::Key key_jm1;
			while (j > 0 && ((key_jm1 = m_render_items[m_sorted_indices[j - 1]].key) < key_i))
			{
				// Moving down & swapping j with j-1 ( could xor ) 
				m_sorted_indices[j] = m_sorted_indices[j - 1];
				--j;
			}

			// When we swap indices we also swap the pointers in the Queue, this way 
			// looping the Queue is done in a decently cache friendly manner. Indices
			// are still needed for temporal coherency
			m_sorted_indices[j] = m_sorted_indices[i];
		}

		// Ready to be executed!
		m_is_ready = true;
	}

	template <typename ItemType>
	const ItemType* Queue<ItemType>::get_items()const
	{
		if (!m_is_ready)
		{
			camy_error("Tried to retrieve render items before end() has been called, returning null");
			return nullptr;
		}

		if (m_render_items.empty())
			return nullptr;

		return &m_render_items[0];
	}

	template <typename ItemType>
	u32 Queue<ItemType>::get_num_items()const
	{
		if (!m_is_ready)
		{
			camy_error("Tried to retrieve number of render items before end() has been called, returning null");
			return 0;
		}

		return static_cast<u32>(m_render_items.size());
	}

	template <typename ItemType>
	const u32* Queue<ItemType>::get_sorted_indices()const
	{
		if (!m_is_ready)
		{
			camy_error("Tried to retrieve sorted indices before end() has been called, returning null");
			return nullptr;
		}

		if (m_sorted_indices.empty())
			return nullptr;

		return &m_sorted_indices[0];
	}

	template <typename ItemType>
	u32 Queue<ItemType>::get_num_sorted_indices()const
	{
		if (!m_is_ready)
		{
			camy_error("Tried to retrieve number of sorted indices before end() has been called, returning null");
			return 0;
		}

		return static_cast<u32>(m_sorted_indices.size());
	}
}