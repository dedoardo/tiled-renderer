#pragma once

// camy
#include <camy/math.hpp>

// rapidjson
#include <rapidjson/document.h>

namespace camy
{
	// They are kept in a separate namespace to avoid pollution
	namespace importer
	{
		using json_type = rapidjson::Type;

		static const char* metadata_extension{ ".metadata" };
		static const char* data_extension{ ".data" };

		const float default_roughness{ 0.5f };
		const float default_metalness{ 0.02f };
		const float default_ior{ 1.5f }; // non metallic materials are betweeen 1.3 - 1.7
		const float3 default_diffuse_color{ 0.f, 1.f, 0.f };
		const float3 default_specular_color{ 0.f, 1.f, 0.f };

		struct JsonTag
		{
			const char name[255];
			json_type type;
		};

		// Json general tags
		const JsonTag name_tag{ "name", rapidjson::Type::kStringType };
		const JsonTag offset_tag{ "offset", rapidjson::Type::kNumberType };
		const JsonTag size_tag{ "size", rapidjson::Type::kNumberType };
		const JsonTag type_tag{ "type", rapidjson::Type::kStringType };
		const JsonTag color_tag{ "color", rapidjson::Type::kArrayType };

		// Json high level tags
		const JsonTag meshes_tag{ "meshes", rapidjson::Type::kArrayType };
		const JsonTag textures_tag{ "textures", rapidjson::Type::kArrayType };
		const JsonTag materials_tag{ "materials", rapidjson::Type::kArrayType };
		const JsonTag nodes_tag{ "nodes", rapidjson::Type::kArrayType };

		// Json low level tags ( Meshes )
		const JsonTag submeshes_tag{ "submeshes", rapidjson::Type::kArrayType };
		const JsonTag vertex_type_tag{ "vertex_type", rapidjson::Type::kNumberType };
		const JsonTag num_vertices_tag{ "num_vertices", rapidjson::Type::kNumberType };
		const JsonTag num_indices_tag{ "num_indices", rapidjson::Type::kNumberType };
		const JsonTag ext_index_tag{ "ext_index", rapidjson::Type::kTrueType };

		const JsonTag vertex_offset_tag{ "vertex_offset", rapidjson::Type::kNumberType };
		const JsonTag index_offset_tag{ "index_offset", rapidjson::Type::kNumberType };
		const JsonTag index_count_tag{ "index_count", rapidjson::Type::kNumberType };

		// Json low level tags ( Textures )
		const JsonTag format_tag{ "format", rapidjson::Type::kNumberType };
		const JsonTag surfaces_tag{ "surfaces", rapidjson::Type::kArrayType };
		const JsonTag width_tag{ "width", rapidjson::Type::kNumberType };
		const JsonTag height_tag{ "height", rapidjson::Type::kNumberType };
		const JsonTag pitch_tag{ "pitch", rapidjson::Type::kNumberType };

		// Json low level tags ( materials ) 
		const JsonTag color_map_tag{ "color_map", rapidjson::Type::kStringType };
		const JsonTag smoothness_map_tag{ "smoothness_map", rapidjson::Type::kStringType };
		const JsonTag metalness_map_tag{ "metalness_map", rapidjson::Type::kStringType };
		const JsonTag smoothness_tag{ "smoothness", rapidjson::Type::kNumberType };
		const JsonTag metalness_tag{ "metalness", rapidjson::Type::kNumberType };
		const JsonTag ior_tag{ "ior", rapidjson::Type::kNumberType };

		// Json low level tags ( nodes ) 
		const JsonTag parent_tag{ "parent", rapidjson::Type::kStringType };
		const JsonTag position_tag{ "position", rapidjson::Type::kArrayType };
		const JsonTag rotation_tag{ "rotation", rapidjson::Type::kArrayType };
		const JsonTag scale_tag{ "scale", rapidjson::Type::kNumberType };
		const JsonTag model_tag{ "model", rapidjson::Type::kObjectType };
		const JsonTag mesh_tag{ "mesh", rapidjson::Type::kStringType };
		const JsonTag radius_tag{ "radius", rapidjson::Type::kNumberType };
		const JsonTag intensity_tag{ "intensity", rapidjson::Type::kNumberType };
	}
}