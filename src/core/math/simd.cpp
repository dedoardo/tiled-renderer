/* simd.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/math/simd.hpp>

namespace camy
{
	using namespace DirectX;

	// <> Loads
	vector camy_simdcall load(const float2& src)
	{
		return XMLoadFloat2((XMFLOAT2A*)&src);
	}
		
	vector camy_simdcall load(const float3& src)
	{
		return XMLoadFloat3((XMFLOAT3A*)&src);
	}

	vector camy_simdcall load(const float4& src)
	{
		return XMLoadFloat4((XMFLOAT4A*)&src);
	}

	vector camy_simdcall load(float x, float y, float z, float w)
	{
		return XMVectorSet(x, y, z, w);
	}

	matrix camy_simdcall load(const float4x4& src)
	{
		return XMLoadFloat4x4((XMFLOAT4X4A*)&src);
	}

	// <> Stores
	void camy_simdcall store(float2& dest, p_vector src)
	{
		XMStoreFloat2((XMFLOAT2*)&dest, src);
	}

	void camy_simdcall store(float3& dest, p_vector src)
	{
		XMStoreFloat3((XMFLOAT3*)&dest, src);
	}

	void camy_simdcall store(float4& dest, p_vector src)
	{
		XMStoreFloat4((XMFLOAT4*)&dest, src);
	}

	void camy_simdcall store(float4x4& dest, p_matrix src)
	{
		XMStoreFloat4x4((XMFLOAT4X4*)&dest, src);
	}

	float camy_simdcall getx(p_vector vec)
	{
		return XMVectorGetX(vec);
	}

	float camy_simdcall gety(p_vector vec)
	{
		return XMVectorGetY(vec);
	}

	float camy_simdcall getz(p_vector vec)
	{
		return XMVectorGetZ(vec);
	}

	float camy_simdcall getw(p_vector vec)
	{
		return XMVectorGetW(vec);
	}

	// <> Operations
	vector camy_simdcall add(p_vector left, p_vector right)
	{
		return XMVectorAdd(left, right);
	}

	vector camy_simdcall sub(p_vector left, p_vector right)
	{
		return XMVectorSubtract(left, right);
	}

	vector camy_simdcall scale(p_vector vec, float factor)
	{
		return XMVectorScale(vec, factor);
	}

	matrix camy_simdcall mul(p_matrix left, p_matrix right)
	{
		return XMMatrixMultiply(left, right);
	}

	// <> Special vector operations
	vector camy_simdcall scale3(p_vector vec, p_vector factors)
	{
		return _mm_mul_ps(vec, factors);
	}

	vector camy_simdcall normalize2(p_vector src)
	{
		return XMVector2Normalize(src);
	}

	vector camy_simdcall normalize3(p_vector src)
	{
		return XMVector3Normalize(src);
	}

	vector camy_simdcall transform(p_vector vec, p_matrix mat)
	{
		return XMVector3Transform(vec, mat);
	}

	float camy_simdcall dot2(p_vector left, p_vector right)
	{
		return XMVectorGetX(XMVector2Dot(left, right));
	}

	float camy_simdcall dot3(p_vector left, p_vector right)
	{
		return XMVectorGetX(XMVector3Dot(left, right));
	}

	vector camy_simdcall cross3(p_vector left, p_vector right)
	{
		return XMVector3Cross(left, right);
	}

	float camy_simdcall len_squared3(p_vector src)
	{
		return XMVectorGetX(XMVector3Dot(src, src));
	}

	float camy_simdcall len3(p_vector src)
	{
		return std::sqrtf(len_squared3(src));
	}

	// <> Special matrix operations
	matrix camy_simdcall create_rotation(p_vector axis, float radians)
	{
		return XMMatrixRotationAxis(axis, radians);
	}

	matrix camy_simdcall create_rotation(p_vector roll_pitch_yaw)
	{
		return XMMatrixRotationRollPitchYaw(
			XMVectorGetX(roll_pitch_yaw),
			XMVectorGetY(roll_pitch_yaw),
			XMVectorGetZ(roll_pitch_yaw));
	}

	matrix camy_simdcall create_translation(p_vector position)
	{
		return XMMatrixTranslation(
			XMVectorGetX(position),
			XMVectorGetY(position),
			XMVectorGetZ(position));
	}

	matrix camy_simdcall create_scaling(float factor)
	{
		return XMMatrixScaling(factor, factor, factor);
	}

	matrix camy_simdcall create_perspective(float fov, float ratio, float near_z, float far_z)
	{
		return XMMatrixPerspectiveFovLH(fov, ratio, near_z, far_z);
	}

	matrix camy_simdcall create_orthogonal(float left, float right, float top, float bottom, float near_z, float far_z)
	{
		return XMMatrixOrthographicOffCenterLH(left, right, bottom, top, near_z, far_z);
	}

	matrix camy_simdcall create_look_at(p_vector pos, p_vector at, p_vector up)
	{
		return XMMatrixLookAtLH(pos, at, up);
	}

	matrix camy_simdcall transpose(p_matrix mat)
	{
		return XMMatrixTranspose(mat);
	}

	// <> Extras
	float to_degrees(float radians)
	{
		return radians * 180 / kPi;
	}

	float to_radians(float degrees)
	{
		return degrees * kPi / 180;
	}

	vector camy_simdcall to_degrees(p_vector src)
	{
		return scale(src, 180 / kPi);
	}

	vector camy_simdcall to_radians(p_vector src)
	{
		return scale(src, kPi / 180);
	}

	vector camy_simdcall plane_dot(p_vector plane, p_vector vec)
	{
		return DirectX::XMPlaneDot(plane, vec);
	}
}