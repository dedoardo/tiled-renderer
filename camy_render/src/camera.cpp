// Header
#include <camy_render/camera.hpp>

// C++ STL
#include <algorithm>

namespace
{
	// Todo : should i change type?>
	const DirectX::XMVECTORF32 yaw_axis{ 0.f, 1.f, 0.f };
	const DirectX::XMVECTORF32 pitch_axis{ 1.f, 0.f, 0.f };
	const DirectX::XMVECTORF32 up_axis{ 0.f, 1.f, 0.f };
}

namespace camy
{
	Camera::Camera(float screen_ratio, float fov, float near_z, float far_z) :
		m_ratio{ screen_ratio },
		m_fov{ fov },
		m_near_z{ near_z },
		m_far_z{ far_z },

		m_pitch{ 0.f },
		m_yaw{ 0.f },

		m_is_dirty{ true },
		m_view{ float4x4_default },
		m_view_projection{ float4x4_default }
	{
		math::store(m_position, { 1.f, 1.f, 1.f });
		math::store(m_direction, { -1.f, -1.f, -1.f });

		math::store(m_projection, math::create_perspective(m_fov, m_ratio, m_near_z, m_far_z));
	}

	void Camera::move(float steps)
	{
		math::store(m_position, math::add(math::load(m_position), math::scale(math::load(m_direction), steps)));
		m_is_dirty = true;
	}

	void Camera::strafe(float steps)
	{
		using namespace DirectX;
		const auto strafe_vec = math::scale(math::cross(math::load(m_direction), up_axis), steps);
		math::store(m_position, math::add(math::load(m_position), strafe_vec));
		m_is_dirty = true;
	}

	void Camera::fly(float steps)
	{
		using namespace DirectX;
		math::store(m_position, math::add(math::load(m_position), math::scale(up_axis, steps)));
		m_is_dirty = true;
	}

	void Camera::yaw(float angle_radians)
	{
		using namespace DirectX;
		m_yaw += angle_radians;
		
		math::store(m_direction, math::mul3(math::load(m_direction), math::create_rotation(yaw_axis, angle_radians)));
		m_is_dirty = true;
	}

	void Camera::pitch(float angle_radians)
	{
		using namespace DirectX;

		if (m_pitch + angle_radians >= (60 * XM_PI / 180) ||
			m_pitch + angle_radians <= (-60 * XM_PI / 180))
			return;

		m_pitch += angle_radians;

		const auto strafe_vec = math::cross(math::load(m_direction), up_axis);
		math::store(m_direction, math::mul3(math::load(m_direction), math::create_rotation(strafe_vec, angle_radians)));
		m_is_dirty = true;
	}

	const float4x4& Camera::get_view()const
	{
		using namespace DirectX;

		if (m_is_dirty)
		{
			_recompute();
			m_is_dirty = false;
		}
	
		return m_view;
	}

	const float4x4& Camera::get_projection()const
	{
		return m_projection;
	}

	const float4x4& Camera::get_view_projection()const
	{
		if (m_is_dirty)
		{
			_recompute();
			m_is_dirty = false;
		}

		return m_view_projection;
	}

	const float3& Camera::get_position()const
	{
		return m_position;
	}

	const Plane* Camera::get_frustum_planes()const 
	{
		if (m_is_dirty)
		{
			_recompute();
			m_is_dirty = false;
		}

		return m_frustum_planes;
	}

	void Camera::_recompute()const 
	{
		using namespace DirectX;
		
		const auto position = math::load(m_position);
		const auto lookat = math::add(position, math::load(m_direction));

		math::store(m_view, math::create_look_at(position, lookat, up_axis));
		math::store(m_view_projection, math::mul(math::load(m_view), math::load(m_projection)));

		//
		// http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
		// as far as i've seen everyone pretty much uses this "technique"
		m_frustum_planes[FrustumFace::Left].x = m_view_projection._14 + m_view_projection._11;
		m_frustum_planes[FrustumFace::Left].y = m_view_projection._24 + m_view_projection._21;
		m_frustum_planes[FrustumFace::Left].z = m_view_projection._34 + m_view_projection._31;
		m_frustum_planes[FrustumFace::Left].w = m_view_projection._44 + m_view_projection._41;

		m_frustum_planes[FrustumFace::Right].x = m_view_projection._14 - m_view_projection._11;
		m_frustum_planes[FrustumFace::Right].y = m_view_projection._24 - m_view_projection._21;
		m_frustum_planes[FrustumFace::Right].z = m_view_projection._34 - m_view_projection._31;
		m_frustum_planes[FrustumFace::Right].w = m_view_projection._44 - m_view_projection._41;

		m_frustum_planes[FrustumFace::Bottom].x = m_view_projection._14 + m_view_projection._12;
		m_frustum_planes[FrustumFace::Bottom].y = m_view_projection._24 + m_view_projection._22;
		m_frustum_planes[FrustumFace::Bottom].z = m_view_projection._34 + m_view_projection._32;
		m_frustum_planes[FrustumFace::Bottom].w = m_view_projection._44 + m_view_projection._42;

		m_frustum_planes[FrustumFace::Top].x = m_view_projection._14 - m_view_projection._12;
		m_frustum_planes[FrustumFace::Top].y = m_view_projection._24 - m_view_projection._22;
		m_frustum_planes[FrustumFace::Top].z = m_view_projection._34 - m_view_projection._32;
		m_frustum_planes[FrustumFace::Top].w = m_view_projection._44 - m_view_projection._42;

		m_frustum_planes[FrustumFace::Near].x = m_view_projection._13;
		m_frustum_planes[FrustumFace::Near].y = m_view_projection._23;
		m_frustum_planes[FrustumFace::Near].z = m_view_projection._33;
		m_frustum_planes[FrustumFace::Near].w = m_view_projection._43;

		m_frustum_planes[FrustumFace::Far].x = m_view_projection._14 - m_view_projection._13;
		m_frustum_planes[FrustumFace::Far].y = m_view_projection._24 - m_view_projection._23;
		m_frustum_planes[FrustumFace::Far].z = m_view_projection._34 - m_view_projection._33;
		m_frustum_planes[FrustumFace::Far].w = m_view_projection._44 - m_view_projection._43;


		XMStoreFloat4(&m_frustum_planes[FrustumFace::Left], XMPlaneNormalize(XMLoadFloat4(&m_frustum_planes[FrustumFace::Left])));
		XMStoreFloat4(&m_frustum_planes[FrustumFace::Right], XMPlaneNormalize(XMLoadFloat4(&m_frustum_planes[FrustumFace::Right])));
		XMStoreFloat4(&m_frustum_planes[FrustumFace::Bottom], XMPlaneNormalize(XMLoadFloat4(&m_frustum_planes[FrustumFace::Bottom])));
		XMStoreFloat4(&m_frustum_planes[FrustumFace::Top], XMPlaneNormalize(XMLoadFloat4(&m_frustum_planes[FrustumFace::Top])));
		XMStoreFloat4(&m_frustum_planes[FrustumFace::Near], XMPlaneNormalize(XMLoadFloat4(&m_frustum_planes[FrustumFace::Near])));
		XMStoreFloat4(&m_frustum_planes[FrustumFace::Far], XMPlaneNormalize(XMLoadFloat4(&m_frustum_planes[FrustumFace::Far])));
	}
}