/* types.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/math/types.hpp>

// camy
#include <camy/core/math/ops.hpp>

// libc
#include <math.h>

namespace camy
{
	float2& float2::operator+=(const float2& right)
	{
		x += right.x;
		y += right.y;
		return *this;
	}

	float2& float2::operator-=(const float2& right)
	{
		x -= right.x;
		y -= right.y;
		return *this;
	}

	float2 & float2::operator*=(const float factor)
	{
		x *= factor;
		y *= factor;
		return *this;
	}

	float2 & float2::operator/=(const float factor)
	{
		x /= factor;	
		y /= factor;
		return *this;
	}

	bool operator==(const float2 & left, const float2 & right)
	{
		return left.x == right.x && left.y == right.y;
	}

	float2 operator-(const float2 & right)
	{
		return float2(-right.x, -right.y);
	}

	float2 operator+(const float2 & left, const float2 & right)
	{
		return float2(
			left.x + right.x,
			left.y + right.y
		);
	}

	float2 operator-(const float2 & left, const float2 & right)
	{
		return float2(
			left.x - right.x,
			left.y - right.y
		);
	}

	float2 operator*(const float2 & left, const float factor)
	{
		return float2(
			left.x * factor,
			left.y * factor
		);
	}

	float2 operator/(const float2 & left, const float factor)
	{
		float inv = 1.f / factor;
		return float2(
			left.x * inv,
			left.y * inv
		);
	}

	// <> Float3
	float3& float3::operator+=(const float3& right)
	{
		x += right.x;
		y += right.y;
		z += right.z;
		return *this;
	}

	float3& float3::operator-=(const float3& right)
	{
		x -= right.x;
		y -= right.y;
		z -= right.z;
		return *this;
	}

	float3 & float3::operator*=(const float factor)
	{
		x *= factor;
		y *= factor;
		z *= factor;
		return *this;
	}

	float3 & float3::operator/=(const float factor)
	{
		float inv = 1.f / factor;
		x *= inv;
		y *= inv;
		z *= inv;
		return *this;
	}

	float3 operator-(const float3 & right)
	{
		return float3(-right.x, -right.y, -right.z);
	}

	float3 operator+(const float3 & left, const float3 & right)
	{
		return float3(
			left.x + right.x,
			left.y + right.y,
			left.z + right.z
		);
	}

	float3 operator-(const float3 & left, const float3 & right)
	{
		return float3(
			left.x - right.x,
			left.y - right.y,
			left.z - right.z
		);
	}

	float3 operator*(const float3 & left, const float factor)
	{
		return float3(
			left.x * factor,
			left.y * factor,
			left.z * factor
		);
	}

	float3 operator/(const float3 & left, const float factor)
	{
		float inv = 1.f / factor;
		return float3(
			left.x * inv,
			left.y * inv,
			left.z * inv
		);
	}

	// <> Float4
	float4& float4::operator+=(const float4& right)
	{
		x += right.x;
		y += right.y;
		z += right.z;
		w += right.w;
		return *this;
	}

	float4& float4::operator-=(const float4& right)
	{
		x -= right.x;
		y -= right.y;
		z -= right.z;
		w -= right.w;
		return *this;
	}

	float4 & float4::operator*=(const float factor)
	{
		x *= factor;
		y *= factor;
		z *= factor;
		w *= factor;
		return *this;
	}

	float4 & float4::operator/=(const float factor)
	{
		float inv = 1.f / factor;
		x *= inv;
		y *= inv;
		z *= inv;
		w *= inv;
		return *this;
	}

	float4 operator-(const float4 & right)
	{
		return float4(-right.x, -right.y, -right.z);
	}

	float4 operator+(const float4 & left, const float4 & right)
	{
		return float4(
			left.x + right.x,
			left.y + right.y,
			left.z + right.z,
			left.w + right.w
		);
	}

	float4 operator-(const float4 & left, const float4 & right)
	{
		return float4(
			left.x - right.x,
			left.y - right.y,
			left.z - right.z,
			left.w - right.w
		);
	}

	float4 operator*(const float4 & left, const float factor)
	{
		return float4(
			left.x * factor,
			left.y * factor,
			left.z * factor,
			left.w * factor
		);
	}

	float4 operator/(const float4 & left, const float factor)
	{
		float inv = 1.f / factor;
		return left * inv;
	}

	float3 operator*(const float3x3& mat, const float3& vec)
	{
		return float3(
			dot(mat.rows[0], vec),
			dot(mat.rows[1], vec),
			dot(mat.rows[2], vec)
		);
	}

    float4x4 operator*(const float4x4 & left, const float4x4 & right)
    {
		return mul(left, right);
    }

	float4 operator*(const float4x4& mat, const float4& vec)
	{
		return float4(
			dot(mat.rows[0], vec),
			dot(mat.rows[1], vec),
			dot(mat.rows[2], vec),
			dot(mat.rows[3], vec)
		);
	}
}