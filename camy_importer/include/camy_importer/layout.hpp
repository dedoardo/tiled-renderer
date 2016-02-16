#pragma once

// camy
#include <camy/camy_base.hpp>
#include <camy/resources.hpp>

// render
#include <camy_render/vertex.hpp>
#include <camy_render/shader_common.hpp>

// C++ STL
#include <cstdlib>
#include <string>
#include <vector>

namespace camy
{
	struct ImportedSubMesh
	{
		u32 vertex_offset{ 0 };
		u32 index_offset{ 0 };
		u32 index_count{ 0 };
	};

	struct ImportedMesh
	{
		std::string name;
		float radius;
		u32 num_vertices{ 0 };
		u32 num_indices{ 0 };
		VertexBuffer* vertex_buffers[2]{ nullptr, nullptr };
		IndexBuffer*  index_buffer{ nullptr };

		u32 vertex_attributes{ VertexAttributes_None };
		IndexBuffer::Type index_type{ IndexBuffer::Type::U16 };
		std::vector<ImportedSubMesh> sub_meshes;
		u32 offset{ 0 };
	};

	struct ImportedMaterial
	{
		shaders::Material camy_material;
		Surface* color_map{ nullptr };
		Surface* smoothness_map{ nullptr };
		Surface* metalness_map{ nullptr };
		std::string name;
	};

	struct ImportedRenderableMesh
	{
		ImportedMesh mesh;
		std::vector<ImportedMaterial> materials;
	};

	struct ImportedTexture
	{
		std::string name;
		u32 width{ 0u };
		u32 height{ 0u };
		std::vector<SubSurface> subsurfaces;
		Surface* surface{ nullptr };
	};
}