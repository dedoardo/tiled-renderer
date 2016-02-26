#pragma once

// camy
#include <camy/base.hpp>
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
		
	*/
	struct LooseNodeObject
	{
		LooseNodeObject(const Sphere& bounding_sphere, void* user_data = nullptr) :
			user_data{ user_data },
			bounding_sphere(bounding_sphere) { } 

		// Pointer to something the user can associate with this node
		// It can be changed without any concern
		void*		user_data; 

		void relocate(const Sphere& bounding_sphere);

		const Sphere& get_bounding_sphere()const { return bounding_sphere; }

	protected:
		friend class LooseOctree;

		// Sphere associated with the object
		// Currently in order to change the boundingsphere a remove() + add() has to be done
		Sphere		bounding_sphere;

		// Node the object is part of, cannot be changed manually a reparent() is necessary
		// call reparent() in order to modify it
		LooseNode*  parent{ nullptr };
	};


	/*
		Preallocating all the nodes is not feasible 5 levels imply 32768 nodes 
		for a total of sizeof(LooseNode) * 32768. That's why we use a pool allocator
	*/
	struct LooseNode
	{
		using Allocator = allocators::PagedPoolAllocator<LooseNode, 8 * 8 * 8>;

		/*
			Retrieves all the user_data contained in the LooseObjects inside the frustum_planes.
			Assuming the frustum planes are 6
		*/
		void retrieve_visible(std::vector<void*>& visible_allocator, const Plane* frustum_planes);

		/*
			To be called whenever a LooseNodeObject is repositioned. If it doesn't fit in
			 this very LooseNode it is relocated adding him from the top. We could reinsert
			 moving from node to node, but inserting a node is simpler and probably more efficient ( in a worst case scenario )
		*/
		void relocate(LooseNodeObject* object);

		/*
			Reference to the creating tree, it is needed for relocation / insertion
		*/
		LooseOctree*	tree;

		DirectX::XMFLOAT3 center{ 0.f, 0.f, 0.f };
		
		float half_width{ 0.f };

		LooseNode* parent{ nullptr };

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