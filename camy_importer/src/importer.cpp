	// Header
#include <camy_importer/importer.hpp>

// camy
#include <camy/gpu_backend.hpp>
#include <camy/camy_init.hpp>
#include <camy/resources.hpp>

// render
#include <camy_render/scene.hpp>

// importer
#include <camy_importer/layout.hpp>
#include <camy_importer/tags.hpp>
#include <camy_importer/json_helpers.hpp>

// rapidjson
#include <rapidjson/document.h>

// C++ STL
#include <fstream>
#include <sstream>

namespace camy
{
	bool ImportedScene::load(const std::string& metadata_filename, Scene& scene)
	{
		using namespace rapidjson;
		using namespace importer;
		
		imeshes.clear();
		imaterials.clear();
		itextures.clear();


		// extracting data filename from metadata one
		std::string data_filename = metadata_filename.substr(0, metadata_filename.find(metadata_extension)) + data_extension;

		// Parsing metadata
		std::ifstream metadata_stream(metadata_filename);
		if (metadata_stream.fail())
		{
			camy_error("Failed to open metadata file: ", metadata_filename);
			return false;
		}

		std::stringstream sstream;
		sstream << metadata_stream.rdbuf();
		std::string json{ sstream.str() };

		Document document;
		document.Parse<0>(json.c_str());
		if (document.HasParseError())
		{
			camy_error("Failed to parse: ", metadata_filename);
			camy_error("JSON Error: ", document.GetParseError());
			return false;
		}

		camy_info("Parsing: ", metadata_filename);

		// Making sure all the required components are here, if something is not found, a warning will suffice
		// for simple scene there might not be the need for textures for instance
		if (!contains_valid_tag(document, meshes_tag))
			camy_warning("No meshes found in the document");

		if (!contains_valid_tag(document, textures_tag))
			camy_warning("No textures found in the document: ", metadata_filename);

		if (!contains_valid_tag(document, materials_tag))
			camy_warning("No materials found in the document: ", metadata_filename);

		if (!contains_valid_tag(document, nodes_tag))
			camy_warning("No nodes found in the document: ", metadata_filename);

		/*
			Data is extracted in the following order for logical order and memory ordering ( data file ) :
			1. Meshes
			2. Textures
			3. Materials	( references textures )
			4. Nodes		( references materials, meshes )
		*/

		// Extracting meshes
		u32 num_meshes{ 0u };
		if (contains_valid_tag(document, meshes_tag))
			num_meshes = document[meshes_tag.name].Size();

		for (auto i{ 0u }; i < num_meshes; ++i)
		{
			const auto& mesh{ document[meshes_tag.name][i] };

			if (!is_valid_mesh(mesh)) // Warning issued by function
				continue;

			ImportedMesh imesh;

			// Need atleast positions
			auto vertex_type{ read_u32(mesh, vertex_type_tag.name, VertexAttributes_None) };
			if (!(vertex_type & VertexAttributes_Position))
			{
				camy_warning("Skipping mesh : ", mesh["name"].GetString(), " position is not present in vertex data");
				continue;
			}

			// This is temporary, we only support meshes with all attributes for now, no automatic generation ( it should be done in the preprocess stage,
			// just avoiding some erros
			if (vertex_type == (
				VertexAttributes_TexCoord |
				VertexAttributes_Normal |
				VertexAttributes_Tangent |
				VertexAttributes_Binormal))
			{
				camy_warning("Skipping mesh : ", mesh["name"].GetString(), " not all vertex attributes are present");
				continue;
			}

			imesh.name = mesh[name_tag.name].GetString();

			if (imeshes.find(imesh.name) != imeshes.end())
			{
				camy_warning("Skipping mesh: ", imesh.name, " duplicate found");
				continue;
			}

			imesh.vertex_attributes = mesh[vertex_type_tag.name].GetUint();
			imesh.offset = mesh[offset_tag.name].GetInt();
			imesh.index_type = IndexBuffer::Type::U16;
			imesh.num_vertices = mesh[num_vertices_tag.name].GetUint();
			imesh.num_indices = mesh[num_indices_tag.name].GetUint();

			// 32-bit indices
			if (contains_valid_tag(mesh, ext_index_tag) &&
				mesh[ext_index_tag.name].GetBool()) 
				imesh.index_type = IndexBuffer::Type::U32;

			// Looping through all submeshes
			auto num_submeshes{ 0u };
			if (contains_valid_tag(mesh, submeshes_tag))
				num_submeshes = mesh[submeshes_tag.name].Size();

			for (auto j{ 0u }; j < num_submeshes; ++j)
			{
				const auto& submesh{ mesh[submeshes_tag.name][j] };
				
				if (!is_valid_submesh(submesh))
					continue;

				ImportedSubMesh isub_mesh;
				isub_mesh.vertex_offset = submesh[vertex_offset_tag.name].GetInt();
				isub_mesh.index_offset = submesh[index_offset_tag.name].GetInt();
				isub_mesh.index_count = submesh[index_count_tag.name].GetInt();

				imesh.sub_meshes.push_back(isub_mesh);
			}

			// Adding it to the dictionary
			imeshes[imesh.name] = imesh;
		}

		// Parsing data stream
		std::ifstream data_stream(data_filename, std::ios::binary);
		if (data_stream.fail())
		{
			camy_error("Failed to open data file : ", data_filename);
			return false;
		}

		// Before effectively creating the nodes and adding them to the scene, we need to generate the graphic resources associated
		// with the meshes/textures
		for (auto& kv : imeshes)
		{
			// Moving file pointer to address
			data_stream.seekg(kv.second.offset);

			// Data is laid out as follows :
			// Positions ( if present ) ( they have to be as for now ) ( checked earlier )
			// Vertex data slot 2 
			// indices
			auto slot1_data_size{ sizeof(float3) };
			auto slot2_data_size{ 0 };

			// Remember that order of data is important
			if (kv.second.vertex_attributes & VertexAttributes_Normal)
				slot2_data_size += sizeof(float3);

			if (kv.second.vertex_attributes & VertexAttributes_TexCoord)
				slot2_data_size += sizeof(float2);

			if (kv.second.vertex_attributes & VertexAttributes_Tangent)
				slot2_data_size += sizeof(float3);

			if (kv.second.vertex_attributes & VertexAttributes_Binormal)
				slot2_data_size += sizeof(float3);

			// Todo: This is temporary, but in order to render RenderNodes all the vertex attributes ahve n
			// to be used, in the future, some might be ignored by switching vertex shader ( or interleaving with
			// random data that is not used anyway ), but as for now everything is needed
			if (!(kv.second.vertex_attributes & VertexAttributes_Normal &&
				kv.second.vertex_attributes & VertexAttributes_TexCoord &&
				kv.second.vertex_attributes & VertexAttributes_Tangent &&
				kv.second.vertex_attributes & VertexAttributes_Binormal))
			{
				camy_warning("Skipping mesh resource generation because not all the vertex attributes are present ( temporary )");
				continue;
			}

			char* slot1_data{ new char[slot1_data_size * kv.second.num_vertices] };
			char* slot2_data{ new char[slot2_data_size * kv.second.num_vertices] };

			auto index_size{ sizeof(u16) };
			if (kv.second.index_type == IndexBuffer::Type::U32)
				index_size = sizeof(u32);

			char* index_data{ new char[index_size * kv.second.num_indices] };

			data_stream.read(slot1_data, slot1_data_size * kv.second.num_vertices);
			data_stream.read(slot2_data, slot2_data_size * kv.second.num_vertices);
			data_stream.read(index_data, index_size * kv.second.num_indices);

			if (data_stream.fail())
			{
				camy_error("Failed to read data for mesh: ", kv.second.name);
			}

			// Now that we have all the data we can create the verter/index buffers
			kv.second.vertex_buffers[0] = hidden::gpu.create_vertex_buffer(static_cast<u32>(slot1_data_size), kv.second.num_vertices, slot1_data);
			kv.second.vertex_buffers[1] = hidden::gpu.create_vertex_buffer(static_cast<u32>(slot2_data_size), kv.second.num_vertices, slot2_data);
			kv.second.index_buffer = hidden::gpu.create_index_buffer(kv.second.index_type, kv.second.num_indices, index_data);

			kv.second.radius = compute_radius(reinterpret_cast<const float3*>(slot1_data), kv.second.num_vertices);
/*
			u16* indices = reinterpret_cast<u16*>(index_data);
 			for (auto j{ 0u }; j < kv.second.num_indices; ++j)
				std::cout << indices[j] << std::endl;
*/
			if (kv.second.vertex_buffers[0] == nullptr ||
				kv.second.vertex_buffers[1] == nullptr ||
				kv.second.index_buffer == nullptr)
			{
				camy_error("Failed to create mesh resources for mesh: ", kv.second.name);
				hidden::gpu.safe_dispose(kv.second.vertex_buffers[0]);
				hidden::gpu.safe_dispose(kv.second.vertex_buffers[1]);
				hidden::gpu.safe_dispose(kv.second.index_buffer);
			}

			delete[] slot1_data;
			delete[] slot2_data;
			delete[] index_data;
		}

		// Extracting textures
		auto num_textures{ 0u };
		if (contains_valid_tag(document, textures_tag))
			num_textures = document[textures_tag.name].Size();

		for (auto i{ 0u }; i < document["textures"].Size(); ++i)
		{
			const auto& texture{ document["textures"][i] };
			
			if (!is_valid_texture(texture))
				continue;

			u32 total_data_size{ 0u };

			// The width and height usually and array sizes are derived based on the type of resource,
			// currently only type 0 ( that is a mipmapped texture2D ) is supported, for texture arrays
			// cubemaps additional infos are needed ( todo )
			// we could deduce surfaces width / height based ion the first mipmap, having parameters is better tho

			ImportedTexture itexture;
			itexture.name = texture[name_tag.name].GetString();

			if (itextures.find(itexture.name) != itextures.end())
			{
				camy_warning("Skipping texture: ", itexture.name, " duplicate found");
				continue;
			}

			itexture.width = read_u32(texture, width_tag.name, 0u);
			itexture.height = read_u32(texture, height_tag.name, 0u);

			// Reading subsurfaces ( mipmaps levels or faces )
			for (auto s{ 0u }; s < texture[surfaces_tag.name].Size(); ++s)
			{
				const auto& subsurface{ texture["surfaces"][s] };

				// Todo: make sure that first mipmap level width/ height corresponds to the one specified inthe upper node
				// I have to wait until we implement texture arrays for it to work correctly
				SubSurface ss;
				ss.pitch = read_u32(subsurface, pitch_tag.name, 0u);
				ss.width = read_u32(subsurface, width_tag.name, 0u);
				ss.height = read_u32(subsurface, height_tag.name, 0u);
				ss.size = read_u32(subsurface, size_tag.name, 0u);

				total_data_size += ss.size;

				itexture.subsurfaces.push_back(ss);
			}

			// Deciding format, we currently only support BC3_UNORM
			auto format{ Surface::translate(read_u32(texture, format_tag.name, 0)) };

			if (format != Surface::Format::BC3Unorm)
			{
				camy_warning("Sorry currently only support BC3Unorm for textures");
				continue;
			}

			// Now we know how much data we can read, going for it
			char* data = new char[total_data_size];

			// Offset is to make sure we everything is ordered properly, either way, making sure the current read offset is the same as the one specified
			// We first read meshes, time for textures now
			auto offset{ read_u32(texture, offset_tag.name, 0) };

			if (static_cast<u32>(data_stream.tellg()) != offset)
			{
				camy_warning("Offsets when reading don't match up, something probably went wrong, adjusting offset anyway");
				data_stream.seekg(0, offset);
			}

			data_stream.read(data, total_data_size);

			if (!data_stream)
				camy_warning("Failed reading texture data");

			// Need to set subsurface data ptr
			auto cur_size{ 0u };
			for (auto& ss : itexture.subsurfaces)
			{
				ss.data = data + cur_size;
				cur_size += ss.size;
			}

			// Creating resources
			itexture.surface = hidden::gpu.create_texture2D(format, itexture.width, itexture.height, &itexture.subsurfaces[0], static_cast<u32>(itexture.subsurfaces.size()));
			if (itexture.surface == nullptr)
				camy_warning("Failed to create surface for texture: ", itexture.name);

			delete[] data;

			itextures[itexture.name] = itexture;
		}

		// Extracting materials
		auto num_materials{ 0u };

		if (contains_valid_tag(document, materials_tag))
			num_materials = document[materials_tag.name].Size();

		for (auto i{ 0u }; i < num_materials; ++i)
		{
			const auto& material{ document["materials"][i] };

			if (!is_valid_material(material))
				continue;

			ImportedMaterial imaterial;
			imaterial.name = material[name_tag.name].GetString();

			if (imaterials.find(imaterial.name) != imaterials.end())
			{
				camy_warning("Skipping material: ", imaterial.name, " duplicate found");
				continue;
			}

			imaterial.camy_material.smoothness = read_float(material, smoothness_tag.name, default_roughness);
			imaterial.camy_material.metalness = read_float(material, metalness_tag.name, default_metalness);

			auto ior{ read_float(material, ior_tag.name, default_ior) };
			float rf = (ior - 1) / (ior + 1);
			imaterial.camy_material.normal_reflectance = rf * rf;
 			imaterial.camy_material.base_color = read_float3(material, color_tag.name, default_diffuse_color);

			// Lookin up if any map is being use
			if (material.HasMember(color_map_tag.name))
			{
				if (itextures.find(material[color_map_tag.name].GetString()) != itextures.end())
				{
					const auto& color_map{ itextures[material[color_map_tag.name].GetString()] };
					imaterial.color_map = color_map.surface;

				}
				else
					camy_warning("Material: ", imaterial.name, " invalid reference: ", material[color_map_tag.name].GetString());
			}

			if (material.HasMember(smoothness_map_tag.name))
			{
				if (itextures.find(material[smoothness_map_tag.name].GetString()) != itextures.end())
				{
					const auto& smoothness_map{ itextures[material[smoothness_map_tag.name].GetString()] };
					imaterial.smoothness_map = smoothness_map.surface;
				}
				else
					camy_warning("Material: ", imaterial.name, " invalid reference: ", material[smoothness_map_tag.name].GetString());
			}

			if (material.HasMember(metalness_map_tag.name))
			{
				if (itextures.find(material[metalness_map_tag.name].GetString()) != itextures.end())
				{
					const auto& metalness_map{ itextures[material[metalness_map_tag.name].GetString()] };
					imaterial.metalness_map = metalness_map.surface;
				}
				else 
					camy_warning("Material: ", imaterial.name, " invalid reference: ", material[metalness_map_tag.name].GetString());
			}

			imaterials[imaterial.name] = imaterial;
		}

		// Extracting nodes
		auto num_nodes{ 0u };
		if (contains_valid_tag(document, nodes_tag))
			num_nodes = document[nodes_tag.name].Size();
		
		for (auto i{ 0u }; i < num_nodes; ++i)
		{
			const auto& node{ document[nodes_tag.name][i] };

			// Looking up name
			const char* name{ nullptr };
			if (node.HasMember(name_tag.name))
				name = node[name_tag.name].GetString();

			// Checking what kind of node this is
			if (!node.HasMember(type_tag.name))
			{
				camy_error("Skippping node: ", name, " no type specified");
				continue;
			}

			// Looking up parent, note that currently the hierarchy has to be specified in order, since
			// reparenting hasn't been implemented yet, thus you can't first have
			// NODE1.parent = NODE2
			// and then NODE2 ( Note that this is in the JSON file )
			SceneNode::Type type;
			if (std::strcmp(node[type_tag.name].GetString(), "transform") == 0)
				type = SceneNode::Type::Transform;
			else if (std::strcmp(node[type_tag.name].GetString(), "render") == 0)
				type = SceneNode::Type::Render;
			else if (std::strcmp(node[type_tag.name].GetString(), "light") == 0)
				type = SceneNode::Type::Light;
			else
			{
				camy_error("Skipping node: ", name, " invalid type: ", node[type_tag.name].GetString());
				continue;
			}

			TransformSceneNode* parent{ nullptr };
			if (node.HasMember(parent_tag.name))
			{
				const char* parent_name{ node[parent_tag.name].GetString() };
				parent = scene.get<TransformSceneNode>(parent_name);
				if (parent == nullptr)
					camy_warning("Failed to look up parent of node: ", name, " this is because parent does not exist or hasn't been specified before this very node");
			}

			switch (type)
			{
			case SceneNode::Type::Transform:
			{
				// For the transform node no attributes are actually required, they all default to the origin
				auto tnode{ scene.create_transform(name, parent) };

				tnode->move(math::load(read_float3(node, position_tag.name, float3_default)));
				tnode->rotate(math::load(read_float3(node, rotation_tag.name, float3_default)));
				tnode->scale(read_float(node, scale_tag.name, 1.f));
				tnode->tag_dirty();

				break;
			}
			case SceneNode::Type::Render:
			{
				// Scene nodes need to have a model otherwise it would be impossible to calcualte the bounding volume
				if (!node.HasMember(model_tag.name) ||
					!node[model_tag.name].IsObject())
				{
					camy_warning("Skipping node: ", name, " because render nodes MUST have a model object attribute");
					continue;
				}

				const auto& model{ node[model_tag.name] };
				if (!model.HasMember(mesh_tag.name) ||
					!model[mesh_tag.name].IsString())
				{
					camy_warning("Skipping node: ", name, " because render nodes.model MUST have a mesh attribute");
					continue;
				}

				// Looking up mesh and making sure it exists
				if (imeshes.find(model[mesh_tag.name].GetString()) == imeshes.end())
				{
					camy_warning("Skipping node: ", name, " failed to reference a previous extracted mesh: ", model[mesh_tag.name].GetString());
					continue;
				}

				const auto& imesh{ imeshes[model[mesh_tag.name].GetString()] };

				// Automatically creating parent node
				if (node.HasMember(position_tag.name) ||
					node.HasMember(rotation_tag.name) ||
					node.HasMember(scale_tag.name))
				{
					auto position{ read_float3(node, position_tag.name, float3_default) };
					auto rotation{ read_float3(node, rotation_tag.name, float3_default) };
					auto scale{ read_float(node, scale_tag.name, 1.f) };

					auto tnode{ scene.create_transform(nullptr, parent) };

					tnode->move(math::load(position));
					tnode->rotate(math::load(rotation));
					tnode->scale(scale);
					tnode->tag_dirty();

					parent = tnode;
				}

				// Now that we have the mesh, the last thing we need are the materials, 
				// the number has to be the same as the the number of submeshes 
				// this approach is not ultra-flexible user-side but that's what we have
				if (!model.HasMember(materials_tag.name) ||
					!model[materials_tag.name].IsArray() ||
					model[materials_tag.name].Size() != imesh.sub_meshes.size())
				{
					camy_warning("Skipping node: ", name, " because the number of materials for the model.mesh needs to be valid and the same and the number of submeshes");
					continue;
				}

				// Great finally time to create the render nodes and adding them
				auto rnode{ scene.create_render(imesh.radius, name, parent) };

				rnode->vertex_buffer1 = imesh.vertex_buffers[0];
				rnode->vertex_buffer2 = imesh.vertex_buffers[1];
				rnode->index_buffer = imesh.index_buffer;

				for (auto sm{ 0u }; sm < imesh.sub_meshes.size(); ++sm)
				{
					const auto& submesh{ imesh.sub_meshes[sm] };

					Renderable renderable;
					renderable.draw_info.index_count = submesh.index_count;
					renderable.draw_info.index_offset = submesh.index_offset;
					renderable.draw_info.vertex_offset = submesh.vertex_offset;
					renderable.draw_info.primitive_topology = PrimitiveTopology::TriangleList;

					// Looking up material in the material table
					if (imaterials.find(model[materials_tag.name][sm].GetString()) == imaterials.end())
					{
						camy_warning("Skipping submesh: ", sm, " because the associated material: ", model[materials_tag.name][sm].GetString(), " was not found");
						continue;
					}

					renderable.material = &imaterials[model[materials_tag.name][sm].GetString()].camy_material;
					renderable.color_map = imaterials[model[materials_tag.name][sm].GetString()].color_map;
					renderable.smoothness_map = imaterials[model[materials_tag.name][sm].GetString()].smoothness_map;
					renderable.metalness_map = imaterials[model[materials_tag.name][sm].GetString()].metalness_map;

					renderable.compute_render_feature_set();

					rnode->renderables.push_back(renderable);
				}

				break;
			}

			case SceneNode::Type::Light:
			{
				// First off reading radius or defaulting to 1, can't use read_float because we need to know if we have it or not
				float radius{ 0.f };
				if (node.HasMember(radius_tag.name))
					radius = static_cast<float>(node[radius_tag.name].GetDouble());
				else
				{
					camy_warning("Light radius for node: ", name, " is not specified, defaulting to 1.f");
					radius = 1.f;
				}

				// If a position is specified a transform node is created, this avoids some hassle user side
				if (node.HasMember(position_tag.name))
				{
					auto position{ read_float3(node, position_tag.name, float3_default) };
					auto tnode{ scene.create_transform(nullptr, parent) };

					tnode->move(math::load(position));
					tnode->tag_dirty();

					parent = tnode;
				}

				auto lnode{ scene.create_light(radius, read_float(node, intensity_tag.name, 1.f), name, parent) };

				lnode->set_color(read_float3(node, color_tag.name, { 0.f, 1.f, 0.f })); // Default is 0, 1, 0 this way it can be quickly noticed

				break;

			}
			}
		}

		return true;
	}
}