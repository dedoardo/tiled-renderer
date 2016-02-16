// Header
#include <camy_importer/json_helpers.hpp>

// C++ STL
#include <algorithm>

namespace camy
{
	namespace importer
	{
		float compute_radius(const float3* vertices, u32 num_vertices)
		{
			float max{ 0.f };

			for (auto i{ 0u }; i < num_vertices; ++i)
				max = std::max(max, math::len_squared3(math::load(vertices[i])));

			return std::sqrt(max);
		}

		u32 read_u32(const rapidjson::Value& node, const char* attribute, u32 default_value)
		{
			if (!node.HasMember(attribute) ||
				!node[attribute].IsNumber())
			{
				camy_warning(attribute, " attribute not found or invalid => ", default_value);
				return default_value;
			}

			return static_cast<u32>(node[attribute].GetUint());
		}

		float read_float(const rapidjson::Value& node, const char* attribute, float default_value)
		{
			if (!node.HasMember(attribute) ||
				!node[attribute].IsNumber())
			{
				camy_warning(attribute, " attribute not found or invalid => ", default_value);
				return default_value;
			}

			return static_cast<float>(node[attribute].GetDouble());
		}

		float3 read_float3(const rapidjson::Value& node, const char* attribute, float3 default_value)
		{
			if (!node.HasMember(attribute) ||
				!node[attribute].IsArray() ||
				node[attribute].Size() != 3)
			{
				camy_warning(attribute, " attribute not found or invalid => ", default_value.x , ",", default_value.y, ",", default_value.z);
				return default_value;
			}

			float3 out;
			out.x = static_cast<float>(node[attribute][0].GetDouble());
			out.y = static_cast<float>(node[attribute][1].GetDouble());
			out.z = static_cast<float>(node[attribute][2].GetDouble());
			return out;
		}

		bool contains_valid_tag(const rapidjson::Value& node, const JsonTag & tag)
		{
			if (!node.HasMember(tag.name) ||
				node[tag.name].GetType() != tag.type)
				return false;

			return true;
		}

		bool is_valid_material(const rapidjson::Value& node)
		{
			if (!contains_valid_tag(node, name_tag))
			{
				camy_warning("Invalid material, not all required attributes have been found [name]");
				return false;
			}

			return true;
		}

		bool is_valid_texture(const rapidjson::Value& node)
		{
			if (!contains_valid_tag(node, name_tag) ||
				!contains_valid_tag(node, format_tag) ||
				!contains_valid_tag(node, offset_tag) ||
				!contains_valid_tag(node, surfaces_tag) ||
				!contains_valid_tag(node, width_tag) ||
				!contains_valid_tag(node, height_tag) ||
				!contains_valid_tag(node, surfaces_tag))
			{
				camy_warning("Invalid texture, not all required attributes have been found [name, format, offset, surfaces, width, height]");
				return false;
			}

			return true;
		}

		bool is_valid_mesh(const rapidjson::Value& node)
		{
			if (!contains_valid_tag(node, name_tag) ||
				!contains_valid_tag(node, submeshes_tag) ||
				!contains_valid_tag(node, offset_tag) ||
				!contains_valid_tag(node, vertex_type_tag) ||
				!contains_valid_tag(node, num_vertices_tag) ||
				!contains_valid_tag(node, num_indices_tag))
			{
				camy_warning("Invalid mesh, not all required attributes have been found [name, submeshes, offset, vertex_type, num_vertices, num_indices");
				return false;
			}

			return true;
		}

		bool is_valid_node(const rapidjson::Value& node)
		{
			if (!contains_valid_tag(node, name_tag) ||
				!contains_valid_tag(node, type_tag))
			{
				camy_warning("Invalid node, not all required attributes have been found[name, type]");
				return false;
			}

			return true;
		}

		bool is_valid_submesh(const rapidjson::Value& node)
		{

			if (!contains_valid_tag(node, index_count_tag) ||
				!contains_valid_tag(node, vertex_offset_tag) ||
				!contains_valid_tag(node, index_offset_tag))
			{
				camy_warning("Invalid submesh, not all required attributes have been found[index_count, vertex_offset, index_offset]");
				return false;
			}

			return true;
		}
	}
}