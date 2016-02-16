from dds_loader_python import dds_loader
from dds_loader_python import dxgi_values
import os
import json

class Textures:
	json_tag = "textures"
	name_tag = "name"
	filename_tag = "filename"

	# The structure of the output data is as follows
	# { 
	#	'name':'SampleTexture'
	#	'datafile':'SampleTexture.data'
	#	'type':1
	# 	'format':2
	# 	[ 
	# 		{ 'width':256, 'height':256, 'pitch':256, 'size':1024 } // <= This is a surface 
	#   ]
	# }
	@staticmethod
	def process(textures_data):
		print("Processing texture table")
		print("Entries found : " + str(len(textures_data)))

		textures = []

		for texture in textures_data:
			if texture[Textures.name_tag] and texture[Textures.filename_tag]:
				textures.append(Textures.load(texture[Textures.name_tag], texture[Textures.filename_tag]))
			else:
				print("Non valid entry : " + texture)

		return textures
		
	@staticmethod
	def load(name, filename):
		print("Loading " + name + "[" + filename + "]")
		dds_texture = dds_loader.DDSTexture()
		dds_texture.load(filename)
		print(dds_texture)

		output_filename = filename + ".data"

		with open(output_filename, "wb") as file_stream:
			file_stream.write(dds_texture.data)

		if os.path.getsize(output_filename) != len(dds_texture.data):
			return None

		# After data has been written we can finally compose the json and we are finished 
		texture_data = {
			'name':name,
			'datafile':output_filename,
			'type':dds_texture.type,
			'format':dds_texture.dxgi_format,
			'surfaces':[]
		}

		for surface in dds_texture.surfaces:
			texture_data['surfaces'].append({
					'width':surface.width,
					'height':surface.height,
					'pitch':surface.pitch,
					'size':surface.size
				})

		return texture_data