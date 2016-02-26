#pragma once

// camy
#include <camy/base.hpp>
#include <camy/math.hpp>

// render
#include "geometries.hpp"

namespace camy
{
	/*
		Todo: Add settable projection camera, even tho this is a test camera.
		Throughout the rest of the code i've tried to be careful and used virtual
		very rarely ( if at all ), but here to be honest it makes it very easy to create 
		a custom camera and the two times per frame the get_view_proj() is called won't 
		really make a difference.
	*/
	class Camera
	{
	public:
		enum FrustumFace
		{
			Left = 0,
			Right,
			Bottom,
			Top,
			Near,
			Far
		};

	public:
		/*
			Constructor: Camera
				Default constructor
		*/
		Camera(float screen_ratio, float fov = math::pi / 4, float near_z = 0.1f, float far_z = 100.f);

		/*
			Destructor: ~Camera
				Default destructor
		*/
		virtual ~Camera() = default;

		Camera(const Camera& other) = default;
		Camera& operator=(const Camera& other) = default;

		void move(float steps);
		void strafe(float steps);
		void fly(float steps);
		void yaw(float angle_radians);
		void pitch(float angle_radians);

		virtual const float4x4& get_view()const;
		virtual const float4x4& get_projection()const;
		virtual const float4x4& get_view_projection()const;

		virtual const float3& get_position()const;
		virtual const Plane* get_frustum_planes()const;

		virtual float get_near_z()const { return m_near_z; }
		virtual float get_far_z()const { return m_far_z; }
		virtual float get_ratio()const { return m_ratio; }

	protected:
		float m_ratio;
		float m_fov;
		float m_near_z;
		float m_far_z;

		mutable Plane m_frustum_planes[6];
	
		float3 m_position;
		float3 m_direction;
		float  m_yaw;
		float  m_pitch;

		mutable bool m_is_dirty;
		mutable float4x4 m_view;
		mutable float4x4 m_projection;
		mutable float4x4 m_view_projection;

	private:
		void _recompute()const;
	};
}