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
#include <camy/core/math/simd.hpp>

namespace camy
{
	float2& float2::operator+=(const float2& right)
	{
		store(*this, add(load(*this), load(right)));
		return *this;
	}

	float2& float2::operator-=(const float2& right)
	{
		store(*this, sub(load(*this), load(right)));
		return *this;
	}

	float2 & float2::operator*=(const float factor)
	{
		store(*this, scale(load(*this), factor));
		return *this;
	}

	float2 & float2::operator/=(const float factor)
	{
		store(*this, scale(load(*this), 1.f / factor));
		return *this;
	}

	float float2::len() const
	{
		return len3(load(*this));
	}

	float float2::len_squared() const
	{
		return len_squared3(load(*this));
	}

	float float2::dot(float2 & right) const
	{
		return dot2(load(*this), load(right));
	}

	float2& float2::normalize()
	{
		store(*this, normalize2(load(*this)));
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
		float2 ret;
		store(ret, add(load(left), load(right)));
		return ret;
	}

	float2 operator-(const float2 & left, const float2 & right)
	{
		float2 ret;
		store(ret, sub(load(left), load(right)));
		return ret;
	}

	float2 operator*(const float2 & left, const float factor)
	{
		float2 ret;
		store(ret, scale(load(left), factor));
		return ret;
	}

	float2 operator/(const float2 & left, const float factor)
	{
		float2 ret;
		store(ret, scale(load(left), 1.f / factor));
		return ret;
	}

	// <> Float3
	float3& float3::operator+=(const float3& right)
	{
		store(*this, add(load(*this), load(right)));
		return *this;
	}

	float3& float3::operator-=(const float3& right)
	{
		store(*this, sub(load(*this), load(right)));
		return *this;
	}

	float3 & float3::operator*=(const float factor)
	{
		store(*this, scale(load(*this), factor));
		return *this;
	}

	float3 & float3::operator/=(const float factor)
	{
		store(*this, scale(load(*this), 1.f / factor));
		return *this;
	}

	float float3::len() const
	{
		return len3(load(*this));
	}

	float float3::len_squared() const
	{
		return len_squared3(load(*this));
	}

	float float3::dot(float3 & right) const
	{
		return dot3(load(*this), load(right));
	}

	float3 float3::cross(float3 & right) const
	{
		float3 ret;
		store(ret, cross3(load(*this), load(right)));
		return ret;
	}

	float3& float3::normalize()
	{
		store(*this, normalize3(load(*this)));
		return *this;
	}

	float3& float3::transform(const float4x4 & matrix)
	{
		store(*this, ::camy::transform(load(*this), load(matrix)));
		return *this;
	}

	float3 operator-(const float3 & right)
	{
		return float3(-right.x, -right.y, -right.z);
	}

	float3 operator+(const float3 & left, const float3 & right)
	{
		float3 ret;
		store(ret, add(load(left), load(right)));
		return ret;
	}

	float3 operator-(const float3 & left, const float3 & right)
	{
		float3 ret;
		store(ret, sub(load(left), load(right)));
		return ret;
	}

	float3 operator*(const float3 & left, const float factor)
	{
		float3 ret;
		store(ret, scale(load(left), factor));
		return ret;
	}

	float3 operator/(const float3 & left, const float factor)
	{
		float3 ret;
		store(ret, scale(load(left), 1.f / factor));
		return ret;
	}

	// <> Float4
	float4& float4::operator+=(const float4& right)
	{
		store(*this, add(load(*this), load(right)));
		return *this;
	}

	float4& float4::operator-=(const float4& right)
	{
		store(*this, sub(load(*this), load(right)));
		return *this;
	}

	float4 & float4::operator*=(const float factor)
	{
		store(*this, scale(load(*this), factor));
		return *this;
	}

	float4 & float4::operator/=(const float factor)
	{
		store(*this, scale(load(*this), 1.f / factor));
		return *this;
	}

	float float4::len3() const
	{
		return ::camy::len3(load(*this));
	}

	float float4::len_squared3() const
	{
		return ::camy::len_squared3(load(*this));
	}

	float float4::dot3(float4 & right) const
	{
		return camy::dot3(load(*this), load(right));
	}

	float4 float4::cross3(float4 & right) const
	{
		float4 ret;
		store(ret, ::camy::cross3(load(*this), load(right)));
		return ret;
	}

	float4& float4::normalize3()
	{
		store(*this, ::camy::normalize3(load(*this)));
		return *this;
	}

	float4& float4::transform(const float4x4 & matrix)
	{
		store(*this, ::camy::transform(load(*this), load(matrix)));
		return *this;
	}

	float4 operator-(const float4 & right)
	{
		return float4(-right.x, -right.y, -right.z);
	}

	float4 operator+(const float4 & left, const float4 & right)
	{
		float4 ret;
		store(ret, add(load(left), load(right)));
		return ret;
	}

	float4 operator-(const float4 & left, const float4 & right)
	{
		float4 ret;
		store(ret, sub(load(left), load(right)));
		return ret;
	}

	float4 operator*(const float4 & left, const float factor)
	{
		float4 ret;
		store(ret, scale(load(left), factor));
		return ret;
	}

	float4 operator/(const float4 & left, const float factor)
	{
		float4 ret;
		store(ret, scale(load(left), 1.f / factor));
		return ret;
	}

    float4x4 operator*(const float4x4 & left, const float4x4 & right)
    {
        float4x4 ret;
        store(ret, mul(load(left), load(right)));
        return ret;
    }

	float4x4 & float4x4::transpose()
	{
		matrix t = ::camy::transpose(load(*this));
		::camy::store(*this, t);
		return *this;
	}

	float4x4 & float4x4::to_shader_order()
	{
#			if defined(camy_matrix_shader_order_row_major)
		// Nothing to do here
#			elif defined(camy_matrix_shader_order_col_major)
		transpose();
#			else
#			error No shader matrix order specified
#			endif

		return *this;
	}
}