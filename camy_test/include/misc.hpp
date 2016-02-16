#pragma once

#include <camy_core.hpp>
#include <camy_render.hpp>

camy::VertexSlot1 g_cube_vb_data[]
{
	DirectX::XMFLOAT3{ -0.5f, 1.f, -0.5f },
	DirectX::XMFLOAT3{ -0.5f, 1.f, 0.5f },
	DirectX::XMFLOAT3{ 0.5f, 1.f, 0.5f },
	DirectX::XMFLOAT3{ 0.5f, 1.f, -0.5f },
	DirectX::XMFLOAT3{ -0.5f, 0.f, -0.5f },
	DirectX::XMFLOAT3{ -0.5f, 0.f, 0.5f },
	DirectX::XMFLOAT3{ 0.5f, 0.f, 0.5f },
	DirectX::XMFLOAT3{ 0.5f, 0.f, -0.5f }
};

camy::u16 g_cube_ib_data[]
{
	0, 1, 2, 0, 2, 3,
	4, 5, 6, 4, 6, 7,
	4, 5, 1, 4, 1, 0,
	6, 7, 3, 6, 3, 2,
	4, 0, 3, 4, 3, 7,
	6, 2, 1, 6, 1, 5
};


camy::VertexSlot1 g_plane_vb_data[]
{
	DirectX::XMFLOAT3{ -10.f, 0.f, 10.f },
	DirectX::XMFLOAT3{ 10.f, 0.f, 10.f },
	DirectX::XMFLOAT3{ 10.f, 0.f, -10.f },
	DirectX::XMFLOAT3{ -10.f, 0.f, -10.f }
};

camy::u16 g_plane_ib_data[]
{
	0, 1, 2,
	0, 2, 3,
};