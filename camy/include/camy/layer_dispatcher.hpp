#pragma once

// camy
#include "layers.hpp"

// C++ STL
#include <vector>

namespace camy
{
	class LayerDispatcher final
	{
	public:
		LayerDispatcher() = default;
		~LayerDispatcher() = default;

		// Copying makes sense, there is no problem in it
		LayerDispatcher(const LayerDispatcher& other) = default;
		LayerDispatcher& operator=(const LayerDispatcher& other) = default;

		void add_layer(Layer* render_layer);
		void dispatch();
		void remove_layer(Layer* render_layer);

	private:
		void _validate_layers();

		std::vector<Layer*>	m_layers;
		std::vector<bool>	m_bitmask;
	};
}