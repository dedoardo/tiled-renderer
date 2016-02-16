#pragma once

// camy
#include <camy/camy_base.hpp>
#include <camy/camy_math.hpp>

namespace camy
{
#pragma pack(push, 1)
	using Plane = float4;

	struct Sphere
	{
		DirectX::XMFLOAT3 center;
		float			  radius;
	};
#pragma pack(pop)
}