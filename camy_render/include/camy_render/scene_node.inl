namespace camy
{
	camy_inline void LightSceneNode::relocate()
	{
		spatial_object.parent->relocate(&spatial_object);
	}

	camy_inline void RenderSceneNode::relocate()
	{
		spatial_object.parent->relocate(&spatial_object);
	}

	camy_inline const float4x4* RenderSceneNode::get_global_transform()const
	{
		camy_assert(parent != nullptr, { return nullptr; },
			"Parent of the current node is invalid");

		camy_assert(parent->get_type() == SceneNode::Type::Transform, { return nullptr; },
			"Parent of the current node is not a transform, something went wrong in the creation process");
		
		return static_cast<TransformSceneNode*>(parent)->get_global_transform();
	}
}