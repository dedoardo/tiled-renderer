#pragma once

// camy
#include <camy/base.hpp>
#include <camy/math.hpp>

// importer
#include "tags.hpp"

namespace camy
{
	namespace importer
	{
		// Given a specified set of vertices, computes the radius, assuming 
		// all vertices are in object space 
		float compute_radius(const float3* vertices, u32 num_vertices);
		
		u32 read_u32(const rapidjson::Value& node, const char* attribute, u32 default_value);
		float read_float(const rapidjson::Value& node, const char* attribute, float default_value);
		float3 read_float3(const rapidjson::Value& node, const char* attribute, float3 default_value);
	
		bool contains_valid_tag(const rapidjson::Value& node, const JsonTag& tag);

		bool is_valid_material(const rapidjson::Value& node);
		bool is_valid_texture(const rapidjson::Value& node);
		bool is_valid_mesh(const rapidjson::Value& node);
		bool is_valid_submesh(const rapidjson::Value& node);
		bool is_valid_node(const rapidjson::Value& node);
	}
}