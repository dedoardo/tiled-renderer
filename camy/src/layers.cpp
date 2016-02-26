// Header
#include <camy/layers.hpp>

// camy
#include <camy/queue.hpp>

namespace camy
{
	RenderLayer::RenderLayer(Order order, u32 pass, u32 num_render_queues, u32 queue_size_estimate) :
		Layer::Layer(Type::Render, order, pass),

		m_render_queues{ nullptr },
		m_num_render_queues{ num_render_queues }
	{
		m_render_queues = new Queue<RenderItem>[m_num_render_queues];
	}

	RenderLayer::~RenderLayer()
	{
		safe_release_array(m_render_queues);
	}

	void RenderLayer::begin()
	{
		if (m_state == State::Queueing)
		{
			camy_warning("Can't called begin multiple times");
			return;
		}

		if (m_state == State::Ready)
		{
			camy_warning("Can't call begin while the items have yet to be executed");
			return;
		}

		for (auto i{ 0u }; i < m_num_render_queues; ++i)
			m_render_queues[i].begin();

		m_state = State::Queueing;
	}

	RenderItem* RenderLayer::create_render_item(u32 render_queue)
	{
		camy_assert(render_queue < m_num_render_queues, { return; }, "Render queue does not identify a valid render queue in the current pass | ", render_queue);

		return m_render_queues[render_queue].create_item();
	}

	void RenderLayer::end(u32 render_queue, const ParameterGroup* shared_parameters, Dependency* dependencies, const u32 num_dependencies)
	{
		if (m_state == State::Executed)
		{
			camy_warning("Can't call end() before begin() has been called") ;
			return;
		}

		if (m_state == State::Ready)
		{
			camy_warning("End on all the render queues hasnt been called");
			return;
		}

		camy_assert(render_queue < m_num_render_queues, { return; }, "Render queue does not identify a valid render queue in the current pass | ", render_queue);

		m_render_queues[render_queue].end(shared_parameters, dependencies, num_dependencies);

		bool all_ready{ true };
		for (auto i{ 0u }; i < m_num_render_queues; ++i)
		{
			if (m_render_queues[i].get_state() != Queue<RenderItem>::State::Ready)
			{
				all_ready = false;
				break;
			}
		}

		if (all_ready)
			m_state = State::Ready;
	}

	void RenderLayer::tag_executed()
	{
		for (auto i{ 0u }; i < m_num_render_queues; ++i)
		{
			m_render_queues[i].tag_executed();
			if (m_render_queues[i].get_state() != Queue<RenderItem>::State::Executed)
			{
				camy_warning("Failed to tag queue as executed");
				return;
			}
		}

		m_state = State::Executed;
	}

	ComputeLayer::ComputeLayer(Order order, u32 pass) :
		Layer::Layer(Type::Compute, order, pass)
	{

	}

	void ComputeLayer::begin()
	{
		if (m_state == State::Queueing)
		{
			camy_warning("Can't called begin multiple times");
			return;
		}

		if (m_state == State::Ready)
		{
			camy_warning("Can't call begin while the items have yet to be executed");
			return;
		}

		m_queue.begin();
	
		m_state = State::Queueing;
	}

	ComputeItem* ComputeLayer::create_compute_item()
	{	
		return m_queue.create_item();
	}

	void ComputeLayer::end()
	{
		if (m_state == State::Executed)
		{
			camy_warning("Can't call end() before begin() has been called");
			return;
		}

		if (m_state == State::Ready)
		{
			camy_warning("End on all the computes queues hasnt been called");
			return;
		}

		m_queue.end();
		m_state = State::Ready;
	}

	const Queue<ComputeItem>* ComputeLayer::get_queue()const
	{
		return &m_queue;
	}

	void ComputeLayer::tag_executed()
	{
		m_queue.tag_executed();
		if (m_queue.get_state() != Queue<ComputeItem>::State::Executed)
			camy_warning("Failed to tag the compute queue as executed");
		else
			m_state = State::Executed;
	}

	PostProcessLayer::PostProcessLayer(Order order, u32 pass) :
		Layer::Layer(Type::PostProcess, order, pass),
		m_items{ nullptr },
		m_num_items{ 0u }
	{
		// Postprocess is always ready
		m_state = State::Permanent;
	}

	PostProcessLayer::~PostProcessLayer()
	{
		safe_release_array(m_items);
	}

	void PostProcessLayer::create(const PostProcessItem* items, u32 num_items)
	{
		if (num_items == 0)
		{
			camy_warning("Void call, number of postprocess items is set to 0");
			return;
		}

		safe_release_array(m_items);
	
		m_items = new PostProcessItem[num_items];
		std::memcpy(m_items, items, sizeof(PostProcessItem) * num_items);
		m_num_items = num_items;
	}

	void PostProcessLayer::set_shared_parameters(const ParameterGroup& parameters)
	{
		m_shared_parameters = parameters;
	}

	void PostProcessLayer::tag_executed()
	{
	}
}