#pragma once

// camy
#include <camy/camy_base.hpp>
#include <camy/camy_math.hpp>

namespace camy
{
	/*
		No enum class because we use them for flags, and having 
		a static_cast every flag is kinda stupid
	*/
	enum VertexAttributes
	{
		VertexAttributes_None =		0,
		VertexAttributes_Position =	1,
		VertexAttributes_Normal =	1 << 1,
		VertexAttributes_TexCoord =	1 << 2,
		VertexAttributes_Tangent =	1 << 3,
		VertexAttributes_Binormal =	1 << 4
	};

	using Index = u16;

	/*
		Slot 1 is shared by all kind of renderables, and just contains the 
		position
	*/
	struct VertexSlot1
	{
		float3 position;
	};

	/* 
		This is the slot2 for renderables ( RenderSceneNode )
	*/
	struct RenderVertexSlot2
	{
		float3 normal;
		float2 tex_coord;
		float3 tangent;
		float3 binormal;
	};
}