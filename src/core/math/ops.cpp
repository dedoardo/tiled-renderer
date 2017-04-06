/* ops.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/math/ops.hpp>

namespace camy
{
	float len(const float2& vec)
	{
		return sqrtf(vec.x * vec.x + vec.y * vec.y);
	}

	float len(const float3& vec)
	{
		return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	}

	float len(const float4& vec)
	{
		return sqrtf(dot(vec, vec));
	}

	float len2(const float2& vec)
	{
		return vec.x * vec.x + vec.y * vec.y;
	}

	float len2(const float3& vec)
	{
		return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	}

	float len2(const float4& vec)
	{
		return dot(vec, vec);
	}

	float dot(const float2& a, const float2& b)
	{
		return a.x * b.x + a.y * b.y;
	}
	
	float dot(const float3& a, const float3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	float dot(const float4& a, const float4& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	float3 cross(const float3& a, const float3& b)
	{
		return float3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	float2 normalize(const float2& vec)
	{
		float inv = 1.f / len(vec);
		return float2(
			vec.x * inv,
			vec.y * inv
		);
	}
	
	float3 normalize(const float3& vec)
	{
		float inv = 1.f / len(vec);
		return float3(
			vec.x * inv,
			vec.y * inv,
			vec.z * inv
		);
	}
	
	float4 normalize(const float4& vec)
	{
		float inv = 1.f / len(vec);
		return float4(
			vec.x * inv,
			vec.y * inv,
			vec.z * inv,
			vec.w * inv
		);
	}

	float3 mul(const float3x3& mat, const float3& vec)
	{
		return float3(
			dot(mat.rows[0], vec),
			dot(mat.rows[1], vec),
			dot(mat.rows[2], vec)
		);
	}

	float4   mul(const float4x4& mat, const float4& vec)
	{
		return float4(
			dot(mat.rows[0], vec),
			dot(mat.rows[1], vec),
			dot(mat.rows[2], vec),
			dot(mat.rows[3], vec)
		);
	}
	
	float4x4 mul(const float4x4& left, const float4x4& right)
	{
		float4x4 result;
		float* rf = (float*)&result;
		float* af = (float*)&left;
		float* bf = (float*)&right;
		for (int i = 0; i < 4; ++i) 
		{
			for (int j = 0; j < 4; ++j) 
			{
				rf[i * 4 + j] = 0;
				for (int k = 0; k < 4; ++k) 
					rf[i * 4 + j] += af[i * 4 + k] * bf[k * 4 + j];
			}
		}
		return result;
	}

	float4x4 transpose(const float4x4& mat)
	{
		return float4x4(
			mat.rows[0].x, mat.rows[1].x, mat.rows[2].x, mat.rows[3].x,
			mat.rows[0].y, mat.rows[1].y, mat.rows[2].y, mat.rows[3].y,
			mat.rows[0].z, mat.rows[1].z, mat.rows[2].z, mat.rows[3].z,
			mat.rows[0].w, mat.rows[1].w, mat.rows[2].w, mat.rows[3].w
		);
	}

	float3x3 rotation(const float3& dir, float angle)
	{
		float3 u = normalize(dir);
		float c = cosf(angle);
		float s = sinf(angle);
		float cc = 1.f - c;

		return float3x3(
			c + (u.x * u.x) * cc,     u.y * u.x * cc + u.z * s, u.z * u.x * cc - u.y * s,
			u.x * u.y * cc - u.z * s, c + (u.y * u.y) * cc    , u.z * u.y * cc + u.x * s,
			u.x * u.z * cc + u.y * s, u.y * u.z * cc - u.x * s, c + (u.z * u.z) * cc 
		);
	}

	// http://stackoverflow.com/questions/349050/calculating-a-lookat-matrix
	// Translation + Rotation
	float4x4 look_at(const float3& pos, const float3& at, const float3& up)
	{
		// Building axis system
		float3 dir = at - pos;
		float3 r2 = normalize(dir); // Z 
		float3 r0 = normalize(cross(up, r2)); // X
		float3 r1 = cross(r2, r0); // Y 

		float d0 = dot(r0, -pos);
		float d1 = dot(r1, -pos);
		float d2 = dot(r2, -pos);

		return float4x4(
			r0.x, r1.x, r2.x, 0.f,
			r0.y, r1.y, r2.y, 0.f,
			r0.z, r1.z, r2.z, 0.f,
			d0, d1, d2, 1.f
		);
	}
	
	float4x4 perspective(float fov, float ratio, float near_z, float far_z)
	{
		float sin_fov = sinf(fov * .5f);
		float cos_fov = cosf(fov * .5f);

		float h = cos_fov / sin_fov;
		float w = h / ratio;
		float range = far_z / (far_z - near_z);

		return float4x4(
			w, 0.f, 0.f, 0.f,
			0.f, h, 0.f, 0.f,
			0.f, 0.f, range, 1.f,
			0.f, 0.f, -range * near_z, 1.f
		);
	}
}