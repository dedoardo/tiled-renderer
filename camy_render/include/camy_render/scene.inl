namespace camy
{
	template <typename NodeType>
	NodeType* Scene::get(const char* name)
	{
		static_assert(std::is_base_of<SceneNode, NodeType>::value, "Invalid subnode type");

		camy_assert(m_nodes_map.find(name) == m_nodes_map.end(), { return nullptr; },
			"Failed to find node :", name, " returning null");

		return static_cast<NodeType*>(m_nodes_map[name]);
	}
}