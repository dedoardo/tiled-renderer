#include <intrin.h>
#include "camy_math.hpp"

namespace camy
{
	namespace math
	{
		camy_inline vector load(const float4& vec)
		{
			return DirectX::XMLoadFloat4(&vec);
		}

		camy_inline vector load(const float3& vec)
		{
			return DirectX::XMLoadFloat3(&vec);
		}

		camy_inline matrix load(const float4x4& mat)
		{
			return DirectX::XMLoadFloat4x4(&mat);
		}

		camy_inline void store(float4& vec, p_vector simd_vec)
		{
			DirectX::XMStoreFloat4(&vec, simd_vec);
		}

		camy_inline void store(float3& vec, p_vector simd_vec)
		{
			DirectX::XMStoreFloat3(&vec, simd_vec);
		}

		camy_inline void store(float4x4& mat, p_matrix simd_matrix)
		{
			DirectX::XMStoreFloat4x4(&mat, simd_matrix);
		}

		camy_inline vector add(p_vector left, p_vector right)
		{
			return DirectX::XMVectorAdd(left, right);
		}

		camy_inline vector sub(p_vector left, p_vector right)
		{
			return DirectX::XMVectorSubtract(left, right);
		}

		camy_inline vector scale(p_vector vec, float factor)
		{
			return DirectX::XMVectorScale(vec, factor);
		}

		camy_inline float len_squared3(p_vector vec)
		{
			return DirectX::XMVectorGetX(DirectX::XMVector3Dot(vec, vec));
		}

		camy_inline matrix mul(p_matrix left, p_matrix right)
		{
			return DirectX::XMMatrixMultiply(left, right);
		}

		camy_inline vector mul3(p_vector vec, p_matrix mat)
		{
			return DirectX::XMVector3Transform(vec, mat);
		}

		camy_inline vector cross(p_vector left, p_vector right)
		{
			return DirectX::XMVector3Cross(left, right);
		}

		camy_inline matrix create_rotation(p_vector axis, float radians)
		{
			return DirectX::XMMatrixRotationAxis(axis, radians);
		}

		camy_inline matrix create_rotation(p_vector roll_pitch_yaw)
		{
			using namespace DirectX;
			return XMMatrixRotationRollPitchYaw(
				XMVectorGetX(roll_pitch_yaw),
				XMVectorGetY(roll_pitch_yaw),
				XMVectorGetZ(roll_pitch_yaw));
		}

		camy_inline matrix create_translation(p_vector position)
		{
			using namespace DirectX;
			return XMMatrixTranslation(
				XMVectorGetX(position),
				XMVectorGetY(position),
				XMVectorGetZ(position));
		}

		camy_inline matrix create_scaling(float factor)
		{
			return DirectX::XMMatrixScaling(factor, factor, factor);
		}

		camy_inline matrix create_perspective(float fov, float ratio, float near_z, float far_z)
		{
			return DirectX::XMMatrixPerspectiveFovLH(fov, ratio, near_z, far_z);
		}

		camy_inline matrix create_orthogonal(float left, float right, float top, float bottom, float near_z, float far_z)
		{
			return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, near_z, far_z);
		}

		camy_inline matrix create_look_at(p_vector pos, p_vector at, p_vector up)
		{
			return DirectX::XMMatrixLookAtLH(pos, at, up);
		}

		camy_inline matrix transpose(p_matrix mat)
		{
			return DirectX::XMMatrixTranspose(mat);
		}

		camy_inline matrix invert(p_matrix mat)
		{
			return DirectX::XMMatrixInverse(nullptr, mat);
		}

		camy_inline u32 upper_pow2(u32 value)
		{
			unsigned long fbs{ 0 };
			_BitScanReverse(&fbs, value);

			// Now if value is say 16 we don't need to shift left
			// Note: not assuming overflow, we would need to decide whether 
			// to shift left by one or not, the problem is that the value 
			// returned by ~fbs & value is not 0 - 1 otherwise we could
			// just use it as argument for the shift. We could compute
			// the min/max w/o branching, but still, is overhead we don't really 
			// care about, so for now i'll go with branching
			return fbs + ((~(1 << fbs) & value) ? 1 : 0);
		}
	}
}