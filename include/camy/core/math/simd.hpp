/* simd.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/math/types.hpp>

// Math library
#include <DirectXMath.h>

/*
	DirectX Math based simd wrapper, functions here are called by math.hpp, 
	for performance-critical code it's better to manually use simd data
	rather than using the float* with overloaded operators.

	Currently not all functions are mapped, just the ones that are internally
	used, whenever a new one is needed it's just added here.
*/
namespace camy
{
#define camy_simdcall __vectorcall
	// SIMD Types
	using vector = DirectX::XMVECTOR;
	using matrix = DirectX::XMMATRIX;
	using p_vector = DirectX::FXMVECTOR;
	using p_matrix = DirectX::FXMMATRIX;

	static_assert(sizeof(float2) == sizeof(DirectX::XMFLOAT2), "Not compatible storage types");
	static_assert(sizeof(float3) == sizeof(DirectX::XMFLOAT3), "Not compatible storage types");
	static_assert(sizeof(float4) == sizeof(DirectX::XMFLOAT4), "Not compatible storage types");
	static_assert(sizeof(float4x4) == sizeof(DirectX::XMFLOAT4X4), "Not compatible storage types");
	// SIMD Functions 
	// <> Loads
    camy_api vector camy_simdcall load(const float2& src);
    camy_api vector camy_simdcall load(const float3& src);
    camy_api vector camy_simdcall load(const float4& src);
    camy_api vector camy_simdcall load(float x, float y, float z, float w = 1.f);
    camy_api matrix camy_simdcall load(const float4x4& src);

	// <> Stores
    camy_api void camy_simdcall store(float2& dest, p_vector src);
    camy_api void camy_simdcall store(float3& dest, p_vector src);
    camy_api void camy_simdcall store(float4& dest, p_vector src);
    camy_api void camy_simdcall store(float4x4& dest, p_matrix src);

	// <> Accessors
    camy_api float camy_simdcall getx(p_vector vec);
    camy_api float camy_simdcall gety(p_vector vec);
    camy_api float camy_simdcall getz(p_vector vec);
    camy_api float camy_simdcall getw(p_vector vec);
	
	// <> Operations
    camy_api vector camy_simdcall add(p_vector left, p_vector right);
    camy_api vector camy_simdcall sub(p_vector left, p_vector right);
    camy_api vector camy_simdcall scale(p_vector vec, float factor);
    camy_api matrix camy_simdcall mul(p_matrix left, p_matrix right);

	// <> Special vector operations
    camy_api vector camy_simdcall scale3(p_vector vec, p_vector factors);
    camy_api vector camy_simdcall normalize2(p_vector src);
    camy_api vector camy_simdcall normalize3(p_vector src);
    camy_api vector camy_simdcall transform(p_vector vec, p_matrix mat);
    camy_api float camy_simdcall  dot2(p_vector left, p_vector right);
    camy_api float camy_simdcall  dot3(p_vector left, p_vector right);
    camy_api vector camy_simdcall cross3(p_vector left, p_vector right);
    camy_api float camy_simdcall  len_squared3(p_vector src);
    camy_api float camy_simdcall  len3(p_vector src);

	// <> Special matrix operations
    camy_api matrix camy_simdcall create_rotation(p_vector axis, float radians);
    camy_api matrix camy_simdcall create_rotation(p_vector roll_pitch_yaw);
    camy_api matrix camy_simdcall create_translation(p_vector position);
    camy_api matrix camy_simdcall create_scaling(float factor);
    camy_api matrix camy_simdcall create_perspective(float fov, float ratio, float near_z, float far_z);
    camy_api matrix camy_simdcall create_orthogonal(float left, float right, float top, float bottom, float near_z, float far_z);
    camy_api matrix camy_simdcall create_look_at(p_vector pos, p_vector at, p_vector up);
    camy_api matrix camy_simdcall transpose(p_matrix mat);

	// <> Extras
    camy_api float to_degrees(float radians);
    camy_api float to_radians(float degrees);
    camy_api vector camy_simdcall to_degrees(p_vector src);
    camy_api vector camy_simdcall to_radians(p_vector src);
    camy_api vector camy_simdcall plane_dot(p_vector plane, p_vector vec);
}