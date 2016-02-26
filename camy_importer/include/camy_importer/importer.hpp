#pragma once

// camy
#include <camy/base.hpp>

// importer
#include "layout.hpp"

// C++ STL
#include <unordered_map>

namespace camy
{
	class Scene;

	struct ImportedScene final
	{
		bool load(const std::string& metadata_filename, Scene& scene);

		std::unordered_map<std::string, ImportedMesh>		imeshes;
		std::unordered_map<std::string, ImportedMaterial>	imaterials;
		std::unordered_map<std::string, ImportedTexture>	itextures;
	};
}