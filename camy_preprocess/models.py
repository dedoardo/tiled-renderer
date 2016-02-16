import numpy as np
from collada import *

from pyassimp import *

import ctypes

class Vertex1(ctypes.Structure):
	_fields_ = [
		("position", ctypes.c_float * 3)
		]

class Vertex2(ctypes.Structure):
	_fields_ = [
		("normal", ctypes.c_float * 3),
		("texcoord", ctypes.c_float * 2),
		("tangent", ctypes.c_float * 3),
		("binormal", ctypes.c_float * 3)
	]

Index = ctypes.c_uint

class SubMesh(ctypes.Structure):
	_fields_ = [
		("index_count", ctypes.c_uint),
		("index_offset", ctypes.c_uint),
		("vertex_offset", ctypes.c_uint)
	]

class Material(ctypes.Structure):
	_fields_ = [
		("ambient", ctypes.c_float * 3),
		("diffuse", ctypes.c_float * 3),
		("specular", ctypes.c_float * 3),
		("shininess", ctypes.c_float)
	]

class Models:
	json_tag = "meshes"
	name_tag = "name"
	filename_tag = "filename"

	@staticmethod
	def process_meshes(data, output_stream, current_file_size):
		name = data["name"]
		filename = data[Models.filename_tag]

		print("Reading meshes from " + filename)
		scene = load(filename, 
			processing = postprocess.aiProcess_Triangulate | postprocess.aiProcess_GenSmoothNormals | postprocess.aiProcess_JoinIdenticalVertices | postprocess.aiProcess_CalcTangentSpace)

		# Data is split up into 2 slots, position in the first one and rest in the other
		# this is to allow for interleaved rendering later
		vertices1 = []
		vertices2 = []

		# Currently non-indexed meshes are not supporteds by the camy backend
		indices = []

		sub_meshes = []

		# Reading vertices
		for assimp_mesh in scene.meshes:
			for vertex_index in range(len(assimp_mesh.vertices)):
				vertex1 = Vertex1()
				vertex2 = Vertex2()

				vertex1.position = tuple(assimp_mesh.vertices[vertex_index])

				vertex2.normal   = tuple(assimp_mesh.normals[vertex_index])
				vertex2.texcoord = tuple(assimp_mesh.texturecoords[0][vertex_index][:2	])
				vertex2.tangent  = tuple(assimp_mesh.tangents[vertex_index])
				vertex2.binormal = tuple(assimp_mesh.bitangents[vertex_index])

				vertices1.append(vertex1)
				vertices2.append(vertex2)

		# Reading indices
		mesh_index_offset = 0
		mesh_vertex_offset = 0
		for assimp_mesh in scene.meshes:
			sub_mesh = SubMesh()
			sub_mesh.index_count = len(assimp_mesh.faces) * 3
			sub_mesh.index_offset = mesh_index_offset
			sub_mesh.vertex_offset = mesh_vertex_offset

			for assimp_face in assimp_mesh.faces:
				assert(len(assimp_face) == 3)	
				indices.append(ctypes.c_uint(assimp_face[0]))
				indices.append(ctypes.c_uint(assimp_face[1]))
				indices.append(ctypes.c_uint(assimp_face[2]))

			mesh_vertex_offset += len(assimp_mesh.vertices)
			mesh_index_offset += sub_mesh.index_count
			sub_meshes.append(sub_mesh)

		offset = current_file_size

		# Now we need to write the data to disk and save offsets
		for vertex1 in vertices1:
			output_stream.write(vertex1)

		current_file_size += ctypes.sizeof(Vertex1) * len(vertices1)

		for vertex2 in vertices2:
			output_stream.write(vertex2)

		current_file_size += ctypes.sizeof(Vertex2) * len(vertices2)

		for index in indices:
			output_stream.write(index)

		current_file_size += ctypes.sizeof(Index) * len(indices)

		print("Writing: ", str(current_file_size), " offset: ", str(offset))

		# Which components does the vector contain ? 
		# These values are taken from camy_render/vertex
		vertex_components = 0
		vertex_components |= 1 << 0 # Position
		vertex_components |= 1 << 1 # Nomal
		vertex_components |= 1 << 2 # Texcoord
		vertex_components |= 1 << 3 # Tangent
		vertex_components |= 1 << 4 # Binormal

		assert(len(vertices1) == len(vertices2))

		mesh = {
			"name" : name,
			"offset": offset,
			"vertex_type": vertex_components,
			"num_vertices": len(vertices1),
			"num_indices": len(indices),
			"ext_index":True,
			"submeshes" : []
		}

		for sub_mesh in sub_meshes:
			print("Processed submesh : " + str(sub_mesh.index_count))
			mesh["submeshes"].append({
					"index_count" : int(sub_mesh.index_count),
					"index_offset" : int(sub_mesh.index_offset), 
					"vertex_offset" : int(sub_mesh.vertex_offset)
				})

		print("Processed mesh" + name) 

		release(scene)
		return mesh, current_file_size