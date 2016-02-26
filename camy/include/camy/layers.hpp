#pragma once

// camy
#include "base.hpp"
#include "common_structs.hpp"
#include "queue.hpp"

// C++ STL
#include <vector>

namespace camy
{
	// Forward declaration
	class Context;

	struct RenderItem;
	struct ParameterGroup;

	class Layer
	{
	public:
		/*
			Enum: Layer::Order
			How this pass should be executed compared to the others.
			Note:
			Remember that gaps cannot exist :)
		*/
		enum class Order
		{
			Ordered,	// Has a specific pass N and is executed before the pass N+1 and after the pass N-1
			Unordered,	// Doesn't matter, can be executed whenever needed
		};

		/*
			Shall i use the same Queue::State ? 
		*/
		enum class State
		{
			Executed,
			Queueing,
			Ready,
			Permanent // Exception for PostProcessLayer
		};

	public:
		enum class Type
		{
			Render,
			Compute,
			PostProcess
		};

		Layer(Type type, Order order, u32 pass) : m_type{ type }, m_order{ order }, m_pass{ pass }, m_state{ State::Executed } { }
		virtual ~Layer() = default;
		
		Type get_type()const { return m_type; }
		Order get_order()const { return m_order; }
		u32 get_pass()const { return m_pass; }
		State get_state()const { return m_state; }
		virtual void tag_executed() = 0;

	// Should not absolutely changed after creation, no point in making it protected
	private:
		Type m_type;
		Order m_order;
		u32   m_pass;

	protected:
		State m_state;
	};

	/*
		Topic: RenderLayer
			A RenderLayer is basically a collection of render queues + + pass ordering. It used to be templated with the keysize
			as parameter, but right now we assume a 64-bit key and that's it, there is no reason to complicate
			things even further and with 64 bit register there should be close to 0 overhead

		Note:
			Most of the renderlayer functionalities involves redirecting the calls to the underlying renderqueues,
			a thing that can be done is making everything inline. There is no real need tho for now so it' left like 
			this
	*/
	class RenderLayer final : public Layer
	{
	public:
		/*
			Constructor: RenderLayer
				Creates a new RenderLayer instance with the specified pass and ordering. Note that pass is u32,

				but in reality very few passes are used and remember that gaps *cannot* exist
		*/
		RenderLayer(Order order, u32 pass, u32 num_render_queues, u32 queue_size_estimate = 100);

		/*
			Destructor: ~RenderLayer
				Destroys the current instance releasina all the resources, it does *not* unregister himself from the
				layer dispatcher!
		*/
		~RenderLayer();

		// Some copy constructors are currently deleted, but can be written, this is not done
		// because it takes some time and there is no real need for it
		RenderLayer(RenderLayer& other) = delete;
		RenderLayer& operator=(RenderLayer& other) = delete;

		RenderLayer(RenderLayer&& other) = default;
		RenderLayer& operator=(RenderLayer&& other) = default;

		void begin();

		RenderItem* create_render_item(u32 render_queue);

		void end(u32 render_queue, const ParameterGroup* shared_parameters = nullptr, Dependency* dependencies = nullptr, const u32 num_dependencies = 0);

		const Queue<RenderItem>* get_render_queues()const { return m_render_queues; }
		u32 get_num_render_queues()const { return m_num_render_queues; }

		void tag_executed()override;

	private:
		Queue<RenderItem>* m_render_queues;
		u32			 m_num_render_queues;
	};

	// Compute layer currently does not support multiple queues
	class ComputeLayer final : public Layer
	{
	public:
		ComputeLayer(Order order, u32 pass);
		~ComputeLayer() = default;

		void begin();

		ComputeItem* create_compute_item();

		void end();

		
		const Queue<ComputeItem>* get_queue()const;
		void tag_executed()override;

	private:
		Queue<ComputeItem> m_queue;
	};

	/*
		Class: PostProcessLayer
			A postprocess layer is different than the other layers, since it does not beging and end,
			but is filled once and can be reloaded, it's made of a bunch of compute items that have no 
			key and are orderered based on the submission order. It should not be rebuilt every frame
	*/
	class PostProcessLayer final : public Layer
	{
	public:
		PostProcessLayer(Order order, u32 pass);
		~PostProcessLayer();
		
		// Note: items are copied, feel free to release the memory
		void create(const PostProcessItem* items, u32 num_items);
		void set_shared_parameters(const ParameterGroup& parameters);

		const ParameterGroup* get_shared_parameters()const { return &m_shared_parameters; }

		const PostProcessItem* get_items()const { return m_items; }
		u32 get_num_items()const { return m_num_items; }
		void tag_executed()override;

	private:
		PostProcessItem* m_items;
		u32 m_num_items;
		ParameterGroup m_shared_parameters;
	};
}