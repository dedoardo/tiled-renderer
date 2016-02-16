// Header
#include <camy_render/scene_node.hpp>

// render
#include <camy_render/scene.hpp>
#include <camy_render/shader_common.hpp>

namespace camy
{
	/*
		=============================================================
							SceneNode
		=============================================================
	*/
	SceneNode::SceneNode(Type type) :
		type{ type },
		scene{ nullptr },
		parent{ nullptr }
	{
	
	}

	void SceneNode::reparent(TransformSceneNode* parent)
	{
		scene->reparent(this, parent);
	}

	void SceneNode::destroy()
	{
		scene->destroy(this);
	}

	/*
	=============================================================
						TransformSceneNode
	=============================================================
	*/
	TransformSceneNode::TransformSceneNode() :
		SceneNode::SceneNode(Type::Transform)
	{

	}

	void TransformSceneNode::move(p_vector delta)
	{
		math::store(position, math::add(math::load(position), delta));
		dirty = true;
	}

	void TransformSceneNode::rotate(p_vector delta)
	{
		math::store(rotation, math::add(math::load(rotation), delta));
		dirty = true;
	}

	void TransformSceneNode::scale(float delta)
	{
		scale_factor *= delta;
		dirty = true;
	}

	const float4x4* TransformSceneNode::get_local_transform()const
	{
		if (dirty)
		{
			// S * R * T
			auto transform{ math::create_scaling(scale_factor) };
			transform = math::mul(transform, math::create_rotation(math::load(rotation)));
			transform = math::mul(transform, math::create_translation(math::load(position)));
			math::store(*local_transform, transform);

			dirty = false;
		}

		return local_transform;
	}

	const float4x4* TransformSceneNode::get_global_transform()const
	{
		// There is no dirty here because the global transform is not something that the 
		// node itself is aware of. We just issure a warning, because this is not the intended
		// behavior
		if (dirty)
			camy_warning("Returning a non-updated global transform");

		return global_transform;
	}

	void TransformSceneNode::tag_dirty()
	{
		camy_assert(scene != nullptr, { return; },
			"Failed to tag node dirty, it hasn't been correctly created");

		// Let's not crash when not running in test mode
		if (scene != nullptr)
			scene->tag_dirty(this);

		to_validate = true;
	}

	void TransformSceneNode::_validate_subtree()
	{
		// Updating current global transform
		// Parent not is ALWAYS validated and the root cannot be revalidated
		math::store(*global_transform, math::transpose(math::mul(
			math::load(*parent->get_global_transform()),
			math::load(*get_local_transform())))) ; // get_local_transform() eventually updates the matrix if not done before

		for (auto& child : children)
		{
			if (child->get_type() == SceneNode::Type::Transform)
				static_cast<TransformSceneNode*>(child)->_validate_subtree();

			// If the node is not a transform node a displacement in this very node
			// might result in an invalid space partitioning structure, that's why 
			// we need to potentially update it ( it's not very costly ) this is 
			// done directly from the child node since we have a reference to the 
			// node in the other space.
			// Transform matrices are automagically updated
			if (child->get_type() == SceneNode::Type::Render)
				static_cast<RenderSceneNode*>(child)->relocate();

			if (child->get_type() == SceneNode::Type::Light)
				static_cast<LightSceneNode*>(child)->relocate();
		}

		to_validate = false;
	}

	/*
	============================================================
							LightSceneNode
	============================================================
	*/
	LightSceneNode::LightSceneNode() :
		SceneNode::SceneNode(SceneNode::Type::Light),
		light{ float3_default, 1.f, { 0.f, 1.f, 0.f}, 1.f }
	{
	
	}

	void camy::LightSceneNode::set_color(const float3& color)
	{
		light.color = color;
	}

	/*
	============================================================
							RenderSceneNode
	============================================================
	*/
	void Renderable::compute_render_feature_set(bool enable_emissive, bool enable_ambient_occlusion)
	{
		material->render_feature_set = shaders::RenderFeatureSet_Default;
		
		if (color_map != nullptr) material->render_feature_set |= shaders::RenderFeatureSet_ColorMap;
		if (smoothness_map != nullptr) material->render_feature_set |= shaders::RenderFeatureSet_SmoothnessMap;
		if (metalness_map != nullptr) material->render_feature_set |= shaders::RenderFeatureSet_MetalnessMap;
		if (normal_map != nullptr) material->render_feature_set |= shaders::RenderFeatureSet_BumpMapping;
		if (enable_emissive) material->render_feature_set |= shaders::RenderFeatureSet_Emissive;
		if (enable_ambient_occlusion) material->render_feature_set |= shaders::RenderFeatureSet_AmbientOcclusion;
	}

	RenderSceneNode::RenderSceneNode() :
		SceneNode::SceneNode(Type::Render)  // Rest is default initialized
	{
		
	}
}