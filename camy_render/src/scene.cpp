// Header
#include <camy_render/scene.hpp>

// render
#include <camy_render/camera.hpp>
#include <camy_render/shader_common.hpp>

namespace camy
{
	Scene::Scene(GPUBackend& gpu_backend) :
		m_root{ nullptr },
		m_octree({ 0.f, 0.f, 0.f }, 100.f, 4),

		m_sun_enabled{ false },
		m_sun_color{ colors::golden },
		m_sun_direction{ -1.f, -1.f, -1.f }
	{
		using namespace DirectX;
		m_root = new (m_transform_node_allocator.allocate()) TransformSceneNode();
		m_root->local_transform = m_transforms_allocator.allocate();
		m_root->global_transform = m_transforms_allocator.allocate();
		XMStoreFloat4x4(m_root->local_transform, XMMatrixIdentity());
		XMStoreFloat4x4(m_root->global_transform, XMMatrixIdentity());
	}

	Scene::~Scene()
	{
		
	}

	TerrainSceneNode* Scene::create_terrain(const char* name, TransformSceneNode* parent)
	{
		// Todo : implement ( will be implemented when working on the terrain system )
		return nullptr;
	}

	TransformSceneNode* Scene::create_transform(const char* name, TransformSceneNode* parent)
	{
		if (parent == nullptr)
			parent = m_root;

		// Constructor is called by default
		auto ret{ m_transform_node_allocator.allocate() };

		ret->parent = parent;
		ret->parent->children.push_back(ret);
		ret->scene = this;

		ret->local_transform = m_transforms_allocator.allocate();
		ret->global_transform = m_transforms_allocator.allocate();
		
		// Setting both to identity
		math::store(*ret->local_transform, math::load(float4x4_default));
		math::store(*ret->global_transform, math::load(float4x4_default));

		if (name != nullptr)
			m_nodes_map[name] = ret;

		camy_info("Creating transform node at: (",
			ret->position.x, ":",
			ret->position.y, ":",
			ret->position.z, ")");

		return ret;
	}

	LightSceneNode* Scene::create_light(float radius, float intensity, const char* name, TransformSceneNode* parent)
	{
		if (parent == nullptr)
			parent = m_root;

		Sphere bounding_sphere;
		bounding_sphere.center = parent->position;
		bounding_sphere.radius = radius;

		// if ret is nullptr the allocator will generate the warning and nullptr will be
		// returned, thus we don't need to do any check.
		auto ret{ m_light_node_allocator.allocate(bounding_sphere) };
		ret->parent = parent;
		ret->parent->children.push_back(ret);
		ret->scene = this;

		// The light's position is the same of the parent's node 
		ret->light.position = static_cast<TransformSceneNode*>(parent)->position; // Offset is default to 0
		ret->light.radius = radius;
		ret->light.intensity = intensity;

		// Now it's time to insert the light in the loose octree
		m_octree.add_object(&ret->spatial_object);

		camy_info("Creating light node at: (",
			ret->get_spatial_object().get_bounding_sphere().center.x, ":",
			ret->get_spatial_object().get_bounding_sphere().center.y, ":",
			ret->get_spatial_object().get_bounding_sphere().center.z, ") radius: ",
			radius);

		if (name != nullptr)
			m_nodes_map[name] = ret;

		return ret;
	}

	RenderSceneNode* Scene::create_render(float radius, const char* name, TransformSceneNode* parent)
	{
		if (parent == nullptr)
			parent = m_root;

		Sphere bounding_sphere;
		bounding_sphere.center = parent->position;
		bounding_sphere.radius = radius;

		auto ret{ m_render_node_allocator.allocate(bounding_sphere) };
		
		ret->parent = parent;
		ret->parent->children.push_back(ret);
		ret->scene = this;


		// Finally adding it
		m_octree.add_object(&ret->spatial_object);

		camy_info("Creating render node at: (",
			ret->get_spatial_object().get_bounding_sphere().center.x, ":",
			ret->get_spatial_object().get_bounding_sphere().center.y, ":",
			ret->get_spatial_object().get_bounding_sphere().center.z, ") radius: ",
			radius);

		if (name != nullptr)
			m_nodes_map[name] = ret;

		return ret;
	}

	void Scene::destroy(SceneNode* node)
	{
		switch (node->get_type())
		{
		case SceneNode::Type::Transform:
		{
			auto transform_node{ static_cast<TransformSceneNode*>(node) };

			// When removing transform nodes we also need to recursively remove all the children,
			// when doing this we do it starting from the leaves
			for (auto& children : transform_node->children)
				destroy(children);

			// Now we can finally release all the resources associated with this very nodes
			m_transforms_allocator.deallocate(transform_node->global_transform);
			m_transforms_allocator.deallocate(transform_node->local_transform);
			m_transform_node_allocator.deallocate(transform_node);
			break;
		}
		case SceneNode::Type::Terrain:
			m_terrain_node_allocator.deallocate(static_cast<TerrainSceneNode*>(node));
			break;

		case SceneNode::Type::Render:
			m_render_node_allocator.deallocate(static_cast<RenderSceneNode*>(node));
			break;
	
		case SceneNode::Type::Light:
			m_light_node_allocator.deallocate(static_cast<LightSceneNode*>(node));
			break;
		}
	}

	void Scene::reparent(SceneNode* node, TransformSceneNode* new_parent)
	{
		// Todo : implement ( not really needed atm ) 
	}

	void Scene::tag_dirty(TransformSceneNode* node)
	{
		m_dirty_nodes.push_back(node);
	}
	
	void Scene::set_sun_enabled(bool value)
	{
		if (value == m_sun_enabled)
			camy_warning("Setting sun enabled with value that already corresponds to the current one");

		m_sun_enabled = value;
	}

	bool Scene::is_sun_enabled()const
	{
		return m_sun_enabled;
	}

	void Scene::set_sun_color(const float4& color)
	{
		m_sun_color = color;
	}

	const float4& Scene::get_sun_color()const
	{
		if (!m_sun_enabled)
			camy_warning("Trying to retrieve sun color even tho sun is not enabled");
		
		return m_sun_color;
	}

	void Scene::set_sun_direction(const float3& dir)
	{
		m_sun_direction = dir;
	}

	const float3& Scene::get_sun_direction()const
	{
		if (!m_sun_enabled)
			camy_warning("Trying to retrieve sun direction even tho sun is not enabled");

		return m_sun_direction;
	}

	matrix Scene::compute_sun_view()const
	{
		return math::create_look_at(math::load(float3(40.f, 40.f, 40.f)), math::load(float3(0.f, 0.f, 0.f)), math::load(float3(0.f, 1.f, 0.f)));
	}

	matrix Scene::compute_sun_projection()const
	{
		return math::create_orthogonal(-50, 50, 50, -50, 0.1f, 100.f);
	}

	void Scene::retrieve_visible(const Camera& camera,
		SceneNode**& scene_nodes_out,
		u32& scene_node_count_out)
	{
		// First off we have to process the dirty nodes and eventually update them
		for (auto& dirty_node : m_dirty_nodes)
		{
			// Very likely that the node is not dirty and previous dirty nodes already 
			// revalidated it
			if (!dirty_node->dirty)
				continue;

			// We need to climb up to the highest node who has yet to be validate
			// Note we are assuming that the root node cannot be in the list. This 
			// because it's completely hidden from the user
			// One thing that might happen is that that 
			// Depth 0 => dirty
			// Depth 1 => not dirty
			// Depth 2 => dirty
			// And validation starts from depth 2, what we do is *always* climb the scene
			// graph to the root, this involved some extra costs but is less than revalidating
			// two times everything
			auto top{ dirty_node };
			auto cur{ static_cast<TransformSceneNode*>(dirty_node->parent) }; // Not even checking if this is valid, it should be an invariant
			while (cur != nullptr)
			{
				if (cur->dirty)
					top = cur;
				cur = static_cast<TransformSceneNode*>(cur->parent);
			}

			// Ok, now we have the top dirty node, what we do know is loop down and revalidate 
			// all the transforms, this is done recursively
			top->_validate_subtree();
		}

		void** visibles{ nullptr }; 
		m_octree.retrieve_visible(camera, visibles,  scene_node_count_out);
		scene_nodes_out = reinterpret_cast<SceneNode**>(visibles);

		m_dirty_nodes.clear();
	}	
}