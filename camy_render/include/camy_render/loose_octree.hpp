#pragma once

// camy
#include <camy/camy_base.hpp>
#include <camy/allocators/paged_pool_allocator.hpp>

// render
#include "geometries.hpp"

// C++ STL
#include <vector>

namespace camy
{
	// Forward declarations
	class Camera;
	struct LooseNode;
	class LooseOctree;

	/*
		This structure is read-only,
		Todo: might think about making everythink private and friend class LooseNode
	*/
	struct LooseNodeObject
	{
		// Pointer to something the user can associate with this node
		void*		user_data; 

		Sphere		bounding_sphere;

		// Only to be touched from the LooseOctree
		LooseNode*		parent;
	};

	/*
		Preallocating all the nodes is not feasible 5 levels imply 32768 nodes 
		for a total of sizeof(LooseNode) * 32768. That's why we use a pool allocator
	*/
	struct LooseNode
	{
		using Allocator = allocators::PagedPoolAllocator<LooseNode, 8 * 8 * 8>;

		void retrieve_visible(std::vector<void*>& visible_allocator, const Plane* frustum_planes);
		void relocate(LooseNodeObject* object);

		LooseOctree*	tree; // Needed for reinstertion
		DirectX::XMFLOAT3 center{ 0.f, 0.f, 0.f };
		
		float half_width{ 0.f };

		LooseNode* parent{ nullptr };

		// Todo: can be substituted with custom solution 
		std::vector<LooseNodeObject*> objects;
		LooseNode* children[8];
	};


	class LooseOctree final
	{
	public:
		LooseOctree(const DirectX::XMFLOAT3& center, float half_width, u32 max_depth);
		~LooseOctree();

		LooseOctree(const LooseOctree& other) = delete;
		LooseOctree& operator=(const LooseOctree& other) = delete;

		LooseOctree(LooseOctree&& other) = delete;
		LooseOctree& operator=(LooseOctree&& other) = delete;

		/*
			Method: add_object
				Adds an object to the octree
		*/
		void add_object(LooseNodeObject* object);

		/*
			Method: retrieve_visible
				This is called once a frame and uses a temporary linear allocator
				to store the nodes, a subsequent call to retrieve visible will 
				invalidate the pointer
		*/
		void retrieve_visible(const Camera& camera, void**& object_array_out, u32& object_count_out);

	private:
		LooseNode* m_root;
		u32		   m_max_depth;

		LooseNode::Allocator		  m_node_allocator;
		std::vector<void*> m_visible_allocator;
	};
}