/* ops.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/math/types.hpp>

namespace camy
{
	float len(const float2& vec);
	float len(const float3& vec);
	float len(const float4& vec);

	float len2(const float2& vec);
	float len2(const float3& vec);
	float len2(const float4& vec);

	float dot(const float2& a, const float2& b);
	float dot(const float3& a, const float3& b);
	float dot(const float4& a, const float4& b);

	float3 cross(const float3& a, const float3& b);

	float2 normalize(const float2& vec);
	float3 normalize(const float3& vec);
	float4 normalize(const float4& vec);

	float3   mul(const float3x3& mat, const float3& vec);
	float4   mul(const float4x4& mat, const float4& vec);
	float4x4 mul(const float4x4& left, const float4x4& right);
	float4x4 transpose(const float4x4& mat);

	float3x3 rotation(const float3& dir, float angle);
	float4x4 look_at(const float3& pos, const float3& at, const float3& up);
	float4x4 perspective(float fov, float ratio, float near_z, float far_z);
}