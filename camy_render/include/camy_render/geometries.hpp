#pragma once

// camy
#include <camy/base.hpp>
#include <camy/math.hpp>

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