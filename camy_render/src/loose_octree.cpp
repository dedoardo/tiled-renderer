// Header
#include <camy_render/loose_octree.hpp>

// render
#include <camy_render/camera.hpp>

namespace camy
{
	void LooseNodeObject::relocate(const Sphere& bounding_sphere)
	{
		this->bounding_sphere = bounding_sphere;
	
		if (parent == nullptr)
		{
			camy_warning("Trying to relocate a node not properly initialized");
			return;
		}

		parent->relocate(this);
	}

	void LooseNode::relocate(LooseNodeObject* object)
	{
		camy_assert(std::find(objects.begin(), objects.end(), object) != objects.end(),
		{ return; }, "Trying to relocate a node that is not part of this loose node");

		// Currently we don't support resizing bounding volume, thus we are not going deeper than the current 
		// node when relocating.
		float3 delta;
		math::store(delta, math::sub(math::load(object->get_bounding_sphere().center), math::load(center)));
		
		// Checking if any of the delta is > half_width, we do this without branching,
		// by removing the decimal value
		u32 count{ 0 };
		count += static_cast<u32>(delta.x / half_width);
		count += static_cast<u32>(delta.y / half_width);
		count += static_cast<u32>(delta.z / half_width);
		
		/*
			If the node moved from the current object we could simply try and go from parent to children
			and keep going this way, but in the end, the cost of adding a node to the octree is very low
			and in a "preallocated" future it might aswell go back to O(1), this way, we simply call a readd
		*/
		if (count > 0)
		{
			// Removing from objects, swapping with last to avoid shifting all objects in memory
			for (auto i{ 0u }; i < objects.size(); ++i)
			{
				if (objects[i] == object)
				{
					// Swapping with itself is not a problem at all
					std::swap(objects[i], objects[objects.size() - 1]);
					break;
				}
			}

			objects.pop_back();

			// Reinserting from the top
			tree->add_object(object);
		}
	}

	camy_inline bool is_contained(const Plane* frustum_planes, DirectX::XMFLOAT3& center, float half_width)
	{
		using namespace DirectX;

		// For all the plane we are checking all the edged, it can be 
		for (auto p{ 0u }; p < 6; ++p)
		{
			const Plane* plane{ &frustum_planes[p] };
			XMVECTOR plane_vec{ XMLoadFloat4(plane) };

			// Todo: could precompute the corners
			XMVECTOR cur_point;
			auto in_count{ 0u };

			// Checking against relaxed bounds all the 8 node's vertices
			cur_point = XMVectorSet(center.x - half_width * 2, center.y - half_width * 2, center.z - half_width * 2, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x - half_width * 2, center.y - half_width * 2, center.z, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x, center.y - half_width * 2, center.z - half_width * 2, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x - half_width * 2, center.y, center.z - half_width * 2, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x + half_width * 2, center.y + half_width * 2, center.z + half_width * 2, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x + half_width * 2, center.y + half_width * 2, center.z, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x, center.y + half_width * 2, center.z + half_width * 2, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			cur_point = XMVectorSet(center.x + half_width * 2, center.y, center.z + half_width * 2, 1.f);
			if (XMVectorGetX(XMPlaneDot(plane_vec, cur_point)) > 0.f)
				++in_count;

			// All points are outside the plane
			if (in_count == 0)
				return false;
		}

		return true;
	}

	void LooseNode::retrieve_visible(std::vector<void*>& visible_allocator, const Plane* frustum_planes)
	{
		// Checking for all his children if any of them intersects with the camera
		for (auto i{ 0u }; i < 8; ++i)
		{
			if (children[i] != nullptr)
			{
				if (is_contained(frustum_planes, children[i]->center, children[i]->half_width))
					children[i]->retrieve_visible(visible_allocator, frustum_planes);
			}
		}

		// Adding the curren't node children
		for (const auto& object : objects)
			visible_allocator.push_back(object->user_data);
	}

	LooseOctree::LooseOctree(const DirectX::XMFLOAT3& center, float half_width, u32 max_depth) : 
		m_root{ nullptr },
		m_max_depth{ max_depth }
	{
		m_root = new (m_node_allocator.allocate()) LooseNode;
		m_root->center = center;
		m_root->half_width = half_width;
		m_root->parent = nullptr;
		m_root->tree = this;
		for (auto i{ 0u }; i < 8; ++i) m_root->children[i] = nullptr;
	}

	LooseOctree::~LooseOctree()
	{

	}

	void LooseOctree::add_object(LooseNodeObject* object)
	{
		if (object == nullptr)
		{
			camy_error("Tried to add null object to loose octree");
			return;
		}

		// When inserting into loose octrees we already know the depth the object will be at,
		// we first have to calculate it
		auto cur_half_width{ m_root->half_width };
		auto depth{ 0u };
		while (object->bounding_sphere.radius <= cur_half_width && depth <= m_max_depth)
		{
			cur_half_width /= 2;
			++depth;
		}
		
		// The object doesn't even fit in the current node, adding to the root
		if (depth == 0)
		{
			m_root->objects.push_back(object);
			return;
		}

		// Off by 1
		cur_half_width *= 2;
		--depth;

		// Insertion time can be brought down to O(1) but this requires preallocating all the 
		// nodes and then calculating the index, this is not really feasible, for depth > 7 numbers
		// are unmanagable
		/*
			Ok here follows a fancy way to calculate octree children index based on 
			we first calculate a number that goes from 0 to total_width / cur_width, 
			[ From now on i'll talk just in 1D , apply 3 times ]
			say that we have
			| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | where each block corresponds to a node at depth
			and our object is located in 6 
			| 0 | 1 | 2 | 3 | 4 | 5 | O | 7
			object.center.x / cur_widht = 6 
			6 = 1 1 0
			these 3 numbers will represent our 3 indices for successive children
			at depth 1 we need to decide whether to go from 0 - 3 or 4 - 7
			6 = [1] 1 0 we then go right 4 - 7
			depth = 2 4-5 or 6-7 ? 
			6 = 1 [1] 0 we then go right 6-7
			depth = 3 6 or 7 ?
			6 = 1 1 [0] we now choose 6 
			The good thing about this is that avoids any kind of branching

			Note : Todo : right now i'm assuming that all indices are correct, need a 
			check on size of the object
		*/
		auto current_node{ m_root };
		
		auto x{ static_cast<u32>((object->bounding_sphere.center.x - m_root->center.x + m_root->half_width)  / cur_half_width) };
		auto y{ static_cast<u32>((object->bounding_sphere.center.y + m_root->center.y + m_root->half_width) / cur_half_width) };
		auto z{ static_cast<u32>((object->bounding_sphere.center.z + m_root->center.z + m_root->half_width) / cur_half_width) };
		
		for (auto i{ 0u }; i <= depth; ++i)
		{
			// Shifting to obtain children index
			auto shift{ depth - i };
			auto children_index{ 0u };

			auto vx{ x >> shift };
			auto vy{ y >> shift };
			auto vz{ z >> shift };

			// i should be able to substitute |= with += ( Todo )
			children_index |= (vx);
			children_index |= (vy) << 1;
			children_index |= (vz) << 2;
			
			// The probability of this branch being hit is way higher than the else
			if (current_node->children[children_index] != nullptr)
				current_node = current_node->children[children_index];
			else
			{
				current_node->children[children_index] = new (m_node_allocator.allocate()) LooseNode;

				auto new_half_width{ m_root->half_width / (2 * (i+1)) };

				current_node->children[children_index]->center.x = current_node->center.x - new_half_width + (vx * new_half_width * 2);
				current_node->children[children_index]->center.y = current_node->center.y - new_half_width + (vy * new_half_width * 2);
				current_node->children[children_index]->center.z = current_node->center.z - new_half_width + (vz * new_half_width * 2);

				current_node->children[children_index]->half_width = new_half_width;
				current_node->children[children_index]->parent = current_node;
				current_node->children[children_index]->tree = this;
				for (auto j{ 0u }; j < 8; ++j) current_node->children[children_index]->children[j] = nullptr;

				current_node = current_node->children[children_index];
			}

			// Removing first indexable bit
			x ^= (vx << shift);
			y ^= (vy << shift);
			z ^= (vz << shift);
		}
		
		// Adding object
		current_node->objects.push_back(object);
		object->parent = current_node;
	}

	void LooseOctree::retrieve_visible(const Camera& camera, void**& object_array_out, u32& object_count_out)
	{
		object_count_out = 0u;

		// Resetting allocator
		m_visible_allocator.clear();
		object_array_out = nullptr;

		// Getting camera planes
		auto frustum_planes{ camera.get_frustum_planes() };
	
		m_root->retrieve_visible(m_visible_allocator, frustum_planes);

		object_count_out = static_cast<u32>(m_visible_allocator.size());

		if (object_count_out > 0)
			object_array_out = &m_visible_allocator[0];
	}
}