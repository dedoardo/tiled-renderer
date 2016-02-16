// Header
#include <camy/layers.hpp>

// camy
#include <camy/queue.hpp>

namespace camy
{
	RenderLayer::RenderLayer(Order order, u32 pass, u32 num_render_queues, u32 queue_size_estimate) :
		Layer::Layer(Type::Render, order, pass),

		m_ready_count{ 0u },

		m_render_queues{ nullptr },
		m_num_render_queues{ num_render_queues }
	{
		m_render_queues = new Queue<RenderItem>[m_num_render_queues];
	}

	RenderLayer::~RenderLayer()
	{
		delete[] m_render_queues;
	}

	void RenderLayer::begin(u32 render_queue)
	{
		// Todo: implement assert with debug message ( kinda like a debug if we were talking about)s
		camy_assert(render_queue < m_num_render_queues, { return; }, "Render queue does not identify a valid render queue in the current pass | ", render_queue);

		m_ready_count = 0;

		// Calling two times begin on the same renderqueue will issue warnings are reclear the buffers ( no major errors ),
		// however we don't care, renderqueues handle that themselves
		m_render_queues[render_queue].begin();
	}

	RenderItem* RenderLayer::create_render_item(u32 render_queue)
	{
		camy_assert(render_queue < m_num_render_queues, { return; }, "Render queue does not identify a valid render queue in the current pass | ", render_queue);

		return m_render_queues[render_queue].create_item();
	}

	void RenderLayer::end(u32 render_queue, const ParameterGroup* shared_parameters, Dependency* dependencies, const u32 num_dependencies)
	{
		camy_assert(render_queue < m_num_render_queues, { return; }, "Render queue does not identify a valid render queue in the current pass | ", render_queue);

		m_render_queues[render_queue].end(shared_parameters, dependencies, num_dependencies);
		++m_ready_count;
	}

	ComputeLayer::ComputeLayer(Order order, u32 pass) :
		Layer::Layer(Type::Compute, order, pass),
		m_is_ready{ true }
	{

	}

	void ComputeLayer::begin()
	{
		if (m_is_ready == false)
		{
			camy_warning("Begin has already been called on a computelayer");
			return;
		}

		m_is_ready = false;
		m_queue.begin();
	}

	ComputeItem* ComputeLayer::create_compute_item()
	{
		if (m_is_ready)
		{
			camy_warning("Tried to queue a compute item, but begin hasnt been called");
			return nullptr;
		}
	
		return m_queue.create_item();
	}

	void ComputeLayer::end()
	{
		m_is_ready = true;
		m_queue.end();
	}

	const Queue<ComputeItem>* ComputeLayer::get_queue()const
	{
		return &m_queue;
	}

	PostProcessLayer::PostProcessLayer(Order order, u32 pass) :
		Layer::Layer(Type::PostProcess, order, pass),
		m_items{ nullptr },
		m_num_items{ 0u }
	{

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

	bool PostProcessLayer::is_ready()const
	{
		if (m_num_items == 0)
			camy_warning("Calling is_ready on an empty postprocesslayer this might be not intended behavior");
		return true;
	}
}