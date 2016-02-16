// Header
#include <camy/layer_dispatcher.hpp>

// camy
#include <camy/camy_init.hpp>

// C++ STL
#undef max
#include <algorithm>

namespace camy
{
	void LayerDispatcher::add_layer(Layer* layer)
	{
		if (layer == nullptr)
		{
			camy_warning("Tried to add null layer");
			return;
		}

		m_layers.push_back(layer);
		_validate_layers();

		camy_info("New layer : pass(", layer->get_pass(), ")");
	}

	void LayerDispatcher::remove_layer(Layer* layer)
	{
		if (layer == nullptr)
		{
			camy_warning("Trying to remove null layer");
			return;
		}

		auto result{ std::find(m_layers.begin(), m_layers.end(), layer) };
		if (result == m_layers.end())
		{
			camy_error("Tried to remove a layer that does not exists");
			return;
		}
		m_layers.erase(result);
		_validate_layers();

		camy_info("Removed layer : pass(", layer->get_pass(), ")");
	}

	void LayerDispatcher::dispatch()
	{
		m_bitmask.clear();
		m_bitmask.resize(m_layers.size(), 0);
		auto current_index{ 0u };
		while (current_index < m_layers.size())
		{
			// Passes with no layers are automatically ignored, warning is
			// issused by the _validate_layers() to warn the user
			auto current_pass{ m_layers[current_index]->get_pass() };

			// Trying to dispatch a layer from the current pass
			auto i{ current_index };
			while (i < m_layers.size() &&
				m_layers[i]->get_order() == RenderLayer::Order::Ordered &&
				m_layers[i]->get_pass() == current_pass && 
				m_bitmask[i] == false)
			{
				if (m_layers[i]->is_ready())
				{
					if (m_layers[i]->get_type() == Layer::Type::Render)
						hidden::gpu.execute(static_cast<const RenderLayer*>(m_layers[i]));
					else if (m_layers[i]->get_type() == Layer::Type::Compute)
						hidden::gpu.execute(static_cast<const ComputeLayer*>(m_layers[i]));
					else if (m_layers[i]->get_type() == Layer::Type::PostProcess)
						hidden::gpu.execute(static_cast<const PostProcessLayer*>(m_layers[i]));
					m_bitmask[i] = true;
					++i;
				}
			}

			// Checking if all layers of current pass have been dispatched
			bool all_dispatched{ true };
			for (auto j{ current_index }; j < i; ++j)
			{
				if (m_bitmask[j] == false)
				{
					all_dispatched = false;
					break;
				}
			}

			// Moving to the next pass
			if (all_dispatched)
				current_index = i;
		}
	}

	void LayerDispatcher::_validate_layers()
	{
		// Sorting the layers by pass
		std::sort(m_layers.begin(), m_layers.end(), [](Layer* left, Layer* right)
		{
			return left->get_pass() < right->get_pass();
		});

		// Making sure there are no empty spots, say that :
		// 0 => shadow_pass
		// 2 => forward
		// 1 is empty, it will be skipped, but we are warning the user this is not intended behavior probably
		// For this we only consider ordered layers
		// If we only have between layers() warning will come out, simply add empty layers to remove them
		auto mask{ 0u };
		u32 max_pass{ 0u };
		for (const auto& layer : m_layers)
		{
			mask |= layer->get_order() == Layer::Order::Ordered ? (1 << layer->get_pass()) : 0x0;
			max_pass = std::max(max_pass, layer->get_pass());
		}

		for (auto i{ 0u }; i < max_pass; ++i)
		{
			if (!(mask & (1 << i)))
				camy_warning("Unwanted behavior, empty render layer slot : ", std::to_string(i).c_str());
		}
	}
}