#pragma once

// camy
#include <camy/common_structs.hpp>

// render
#include "loose_octree.hpp"
#include "shader_common.hpp" // Cant' forward declare because of Light :S

namespace camy
{
	/*
		Note:
			The following scene tree implementation concept is taken from :
				http://on-demand.gputechconf.com/gtc/2013/presentations/S3032-Advanced-Scenegraph-Rendering-Pipeline.pdf
			In addition, i've tried to keep everything as cache-friendly as possible, the Scene ( that represents a scene tree)
			has all the data layed out togheter. Key concepts in the following implementation are :
			- Render nodes, Terrain nodes and light nodes are FINAL nodes, they can have no children,
			If you want to inherit a transform from the parent use a transform node. Sample tree
			Root:
			|_ Terrain Node
			|_ Transform Node ( Car )
			  |_ Wheels
			  |_ Body
			  |_ Transform Node ( Siren )
				|_ Light

			Note: currently the user allocates the node, in a "future" system i might give handles ( indices ) to the user,
			it's easy to implement, but i personally don't really like it, if "performance" will ever become a problem, for sure
			this will be done.
	*/

	// Forward declaration
	class Scene;
	struct TransformSceneNode;

	/*
		There is no virtual destructors because SceneNode and its subclasses are merely data container
		and the destructor WONT be called
		Todo: call the destructor
	*/
	struct SceneNode
	{
		enum class Type
		{
			Terrain,
			Transform,
			Light,
			Render
		};

		// Type is bound at creation, it can't be changed
		SceneNode(Type type);

		// Destructor does nothing allocation / deallocation is all managed by the Scene, this is 
		// merely a container that provides an interface to the user
		~SceneNode() = default;

		// Only transform nodes can be parents
		void reparent(TransformSceneNode* parent);

		// detaches and destroys this very node, if it is a transform node
		// all subnodes will be destroyed aswell
		void destroy();

		Type get_type()const { return type; }

		// Everything here is private, meaning that the user should not touch it for *any reason*,
		// Having a friend class seems kinda "hackish" but the interface is way cleaner.
		// This for instance is not done to loose octree nodes because ther doesn't really have to 
		// interact with them
		// They don't have the m_ prefix because this is not really encapsulation, and either way
		// they are private
	protected:
		friend class Scene;

		// Keeping a reference to the scene is important because modifying a node ( transform ) 
		// requires a revalidation of its subtree before rendering
		Scene*	   scene;
		TransformSceneNode* parent;
		
		const Type type;
	};

	/*
		Terrain is separated and for culling could be put in a quadtree or some other kind
		of space partitioning structure, having it in the octree is kinda useless ( especially since
		it's loose and we are not taking at all advantage of the looseness with such big nodes )
	*/
	struct TerrainSceneNode : public SceneNode
	{
		// TODO : 
	};
	
	struct TransformSceneNode final : public SceneNode
	{
		TransformSceneNode();
		~TransformSceneNode() = default;

		// Helper methods that allow for caching  compound transformations
		void move(p_vector delta);
		void rotate(p_vector delta);
		void scale(float delta); // We currently only support uniform scaling, it involves some
									   // troubles down the road, and there is no real need for it 
									   // right now

		const float4x4* get_local_transform()const;
		const float4x4* get_global_transform()const;

		// Once you have finished updating the node it's needed to tag him dirty, tagging him dirty
		// two times is not really a problem and will not result in errors, but should be indeed avoided
		// you might not want to tag_dirty every single frame ( for possibly different reasons   
		void tag_dirty();

	private:
		friend class Scene;

		void _validate_subtree();

		// Only transform nodes can have children
		std::vector<SceneNode*> children;

		float3 position{ float3_default };
		float3 rotation{ float3_default };
		float  scale_factor{ 1.f };

		// Todo : might merge these values in the future, currently one is for the user ( he might need to obtain 
		// the current local, and the other refers to its "transmission" down the graph
		mutable bool dirty{ false };
		bool		 to_validate{ false };
		float4x4*	 local_transform{ nullptr };
		float4x4*	 global_transform{ nullptr };
	};

	/*
		Modifying a light's radius require a reevaluation in the spatial partitioning structure
		this is something that will be done but has not been implemented yet ( Todo ) 
	*/
	struct LightSceneNode final : public SceneNode
	{
		LightSceneNode(const Sphere& bounding_sphere);
		~LightSceneNode() = default;

		void set_color(const float3& color);
		const shaders::Light& get_light()const { return light; }

		camy_inline void relocate();

		const LooseNodeObject& get_spatial_object()const { return spatial_object; }

	private:
		friend class Scene;

		/*
			The reason why this is hidden is that this is the data structure ready to be uploaded onto 
			the cbuffer ( GPU ) and tbh i wanted to avoid further data transformations, since it contains
			the position, it should not be directly manipulated by the user. That is done via parent transform
			nodes and implies some kind of movement in the pspace partitioning data structure aswell.
		*/
		shaders::Light light; 
		LooseNodeObject		spatial_object;
	};

	struct Renderable
	{
		DrawInfo			draw_info;
		shaders::Material*	material{ nullptr };
		Surface*			color_map{ nullptr };
		Surface*			smoothness_map{ nullptr };
		Surface*			metalness_map{ nullptr };
		Surface*			normal_map{ nullptr };
		
		void compute_render_feature_set(bool enable_emissive = false, bool enable_ambient_occlusion = false);
	};

	struct RenderSceneNode final : public SceneNode
	{
		RenderSceneNode(const Sphere& bounding_sphere);
		~RenderSceneNode() = default;
		
		/*
			Interaction between material and light:
				Opaque & Translucent are kept in one queue and sorted by order
				Transparent objects currently dont cast shadows, in the future they will with some implications on shadows.
				The idea behind translucent objects is to render them 
		*/
		enum PhysicalProperty
		{
			Opaque,
			Translucent,
			Transparent
		};

		PhysicalProperty	physical_property{ PhysicalProperty::Opaque };

		VertexBuffer*	vertex_buffer1{ nullptr };
		VertexBuffer*	vertex_buffer2{ nullptr };
		IndexBuffer*	index_buffer{ nullptr };

		std::vector<Renderable> renderables;

		camy_inline void relocate();

		camy_inline const float4x4* get_global_transform()const;

		const LooseNodeObject& get_spatial_object()const { return spatial_object; }


	private:
		friend class Scene;
		LooseNodeObject		spatial_object;
	};
}

#include "scene_node.inl"