# Python
import sys
import json

# Custom
from output import *
from textures import Textures
from models import Models

# Function entrypoint
def main():
	if len(sys.argv) < 2:
		print(OutputError.no_arguments)
		print("Using default sample.json")
		input_file = "sample.json"
	else:
		input_file = sys.argv[1]

	print("Processing : " + input_file)
	with open(input_file) as json_file:
		json_data = json.load(json_file)

	data_out = { 
		"meshes":[],
		"textures": json_data["textures"],
		"materials": json_data["materials"],
		"nodes": json_data["nodes"]
	} 

	# Processing meshes, that will be split into submeshes and all the data
	# will be merged into one single file
	with open(input_file.replace(".json", ".data"), "wb") as file_stream:
		file_size = 0
		for mesh_element in json_data[Models.json_tag]:
			json_element, file_size = Models.process_meshes((mesh_element), file_stream, file_size)
			data_out["meshes"].append(json_element)

	# Writing metadata file out
	with open(input_file.replace(".json", ".metadata"), "w") as file_stream:
		json.dump(data_out, file_stream, indent=4, separators=(',', ': '))

if __name__ == "__main__":
	main()