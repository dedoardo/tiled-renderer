#pragma once

// camy
#include <camy/camy_base.hpp>
#include <camy/common_structs.hpp>

// C++ STL
#include <vector>

namespace camy
{
	// Forward declarations
	struct ParameterGroup;

	/*
		Class: Queue
			This is a Queue, multiple render queues make up a layers, an example would be :
				Forward layer made of :
					opaque queue
					transparent queue
	*/
	template <typename ItemType>
	class Queue final
	{
	public:
		Queue();
		~Queue() = default;

		// Copying is ok, but dont see where it could be called ( inside containers ? )
		Queue(const Queue& other) = default;
		Queue& operator=(const Queue& other) = default;

		/*
			Function: begin
				Prepares before queueing items, to be called before calling queue() it's needed to reset internal buffer,
				undefined behavior otherwise
		*/
		void begin();

		ItemType* create_item();

		/*
			Function: end
				Sorts and does last minute things before actual rendering, once end is called() any subsequent call to
				LayerDispatcher::dispatch() might eventually render the layer depending on ordering
		*/
		void end(const ParameterGroup* shared_parameters = nullptr, Dependency* dependencies = nullptr, const u32 num_dependencies = 0);
	
		/*
			Function: get_shared_parameters
				Returns the shared parameters by all the items in the Queue
		*/
		const ParameterGroup& get_shared_parameters()const { static const ParameterGroup default_shared_parameters; return m_shared_parameters == nullptr ? default_shared_parameters : *m_shared_parameters; }

		/*
			Function: get_dependencies
				Returns the buffer of dependencies that need to be taken care of after rendering all the items,

				Note: This is done at the, after rendering!
		*/
		const Dependency* get_dependencies()const { return m_dependencies; }

		/*
			Function: get_num_dependencies
				Returns the number of the dependencies in the get_dependencies() buffer
		*/
		u32 get_num_dependencies()const { return m_num_dependencies; }

		/*
			Function: get_render_items
				Returns a pointer to the buffer of render items, call get_num_render_items() to see the loop
				boundaries.
		*/
		const ItemType* get_items()const;

		/*
			Function: get_num_render_items
				Returns the number of items that are in queue, this will return 0 until is_ready() is true
		*/
		u32 get_num_items()const;

		/*
			Function: get_sorted_indices
				Returns the sorted index buffer
		*/
		const u32* get_sorted_indices()const;

		/*
			Function: get_num_sorted_indices
				Returns the length of the index buffer, this is the same as num_render_items
		*/
		u32 get_num_sorted_indices()const;

	private:
		// Currently 
		std::vector<ItemType> m_render_items;

		// Not even remotely handling the > 2^32 items queued
		std::vector<u32> m_sorted_indices;

		ParameterGroup const*   m_shared_parameters;
		Dependency*		m_dependencies;
		u32				m_num_dependencies;

		bool			m_is_ready;
	};
}

#include "queue.inl"