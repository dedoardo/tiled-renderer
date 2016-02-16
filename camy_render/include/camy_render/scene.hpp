#pragma once

// camy
#include <camy/allocators/paged_pool_allocator.hpp>

// render
#include "scene_node.hpp"
#include "loose_octree.hpp"

// C++ STL
#include <vector>
#include <functional>
#include <unordered_map>

namespace camy
{
	class Camera;

	/*
		Class: Scene
			Scenegraph used for hierarchical transformation of the nodes, 
			lights are nodes, but since we currently only support one shadow
			casting light, it is kep separated.

			Currently the scene doesn't manage node creation, it used to do it
			and it might in the future, this is a TodoThink:
	*/
	class Scene final
	{
	public:
		Scene(GPUBackend& gpu_backend);
		~Scene();

		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;

		Scene(Scene&& other) = delete;
		Scene& operator=(Scene&& other) = delete;

		// nullptr equals root node.
		// The root node is nothing else than an identity transform node
		// names can be null, if so then they won't be added to the name_map
		TerrainSceneNode*	create_terrain(const char* name, TransformSceneNode* parent = nullptr);
		TransformSceneNode* create_transform(const char* name, TransformSceneNode* parent = nullptr);
		LightSceneNode*		create_light(float radius, float intensity, const char* name, TransformSceneNode* parent = nullptr);
		RenderSceneNode*	create_render(float radius, const char* name, TransformSceneNode* parent = nullptr);

		/*
			Function: getr
				Retrieves the node that has been created with	
		*/
		template <typename NodeType>
		NodeType* get(const char* name);

		/*
			Function: destroy
				Destroys and detached a node from the scenegraph and eventually from any
				space partition structure
		*/
		void destroy(SceneNode* node);

		// This methods are here for clarity, using the ones inside the SceneNode struct
		// is the same as calling them here
		void reparent(SceneNode* node, TransformSceneNode* new_parent = nullptr);
		void tag_dirty(TransformSceneNode* node);

		// Shadow casting light
		void set_sun_enabled(bool value);
		bool is_sun_enabled()const;
		void set_sun_color(const float4& color);
		const float4& get_sun_color()const;
		void set_sun_direction(const float3& dir);
		const float3& get_sun_direction()const;
		matrix compute_sun_view()const;
		matrix compute_sun_projection()const;

		void retrieve_visible(const Camera& camera,
			SceneNode**& scene_nodes_out,
			u32& scene_node_count_out);

	private:
		/*
			In order to give the user pointers to scene nodes and not handles ( or any other redirecting index ) 
			we can't reallocate the memory thus invalidating the pointers. Not having all the
			nodes stored together is aweful cache-wise, what we do is having pages.
			Nodes are stored ( for the most part ) together and without reallocation we can hand
			the user pointers.
			Processing the nodes is still done linearly via the pointer arrays
			If this turns out to be a bottleneck ( highly doubt ) things will change.
			I personally feel this is a tradeoff between an ok interface ( users don't have indices are have to 
			pass through the scene everytime they need a node ) and a memory friendly and efficient design
		*/
		allocators::PagedPoolAllocator<TerrainSceneNode>	m_terrain_node_allocator;
		allocators::PagedPoolAllocator<TransformSceneNode>	m_transform_node_allocator;
		allocators::PagedPoolAllocator<RenderSceneNode>		m_render_node_allocator;
		allocators::PagedPoolAllocator<LightSceneNode>		m_light_node_allocator;
		allocators::PagedPoolAllocator<DirectX::XMFLOAT4X4> m_transforms_allocator;

		/*
			Used to keep track of nodes that need to be reevaluated on a per-frame basis
		*/
		std::vector<TransformSceneNode*> m_dirty_nodes;

		/*
			Root node situated at the origin with no rotation
		*/
		TransformSceneNode*	m_root;
	
		/*
			Utility data structures that allows to retrieve nodes based on their name, 
			this is especially useful when loading scenes externally and then later there is the need
			to reference  nodes from within code. As it is an hashtable it should not be used every 
			time you need to reference a node. Pointers to node can be cached and should be.
		*/
		std::unordered_map<std::string, SceneNode*>  m_nodes_map;

		/*
			Right now we only support one shadow casting light and is situated here, 
			later when there will be more environment settings available it will be 
			separated
		*/
		bool   m_sun_enabled;
		float4 m_sun_color;
		float3 m_sun_direction;

		/*
			Space partitioning structure
		*/
		LooseOctree	m_octree;

	};
}

#include "scene.inl"