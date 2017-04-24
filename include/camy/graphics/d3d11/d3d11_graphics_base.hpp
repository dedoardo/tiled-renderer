/* d3d11_graphics_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>

// Macro for those parameters
#define camy_uuid_ptr(object) __uuidof(std::remove_pointer<decltype(object)>::type), reinterpret_cast<void**>(&object)

struct IUnknown;

// All forward declarations to avoid bloating header files
using WindowHandle = void*;

using GraphicsAPIVersion = camy::uint32;

struct ID3D11Device;
struct ID3D11Device1;
struct ID3D11DeviceContext;
struct ID3D11DeviceContext1;
struct ID3D11CommandList;

struct ID3D11Buffer;
struct ID3D11Texture2D;
struct ID3D11BlendState;
struct ID3D11RasterizerState;
struct ID3D11InputLayout;
struct ID3D11SamplerState;
struct ID3D11DeviceChild;
struct ID3D11DepthStencilState;
struct ID3D11UnorderedAccessView;

struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

struct IDXGIFactory;
struct IDXGIAdapter;
struct IDXGISwapChain;

namespace camy
{
	struct NativeSurface
	{
		ID3D11Texture2D*			texture2d = nullptr;
		ID3D11ShaderResourceView**	srvs = nullptr;
		ID3D11RenderTargetView**	rtvs = nullptr;
		ID3D11DepthStencilView**	dsvs = nullptr;
		ID3D11UnorderedAccessView**	uavs = nullptr;
		rsize num_views = 0;
	};

	struct NativeBuffer
	{
		ID3D11Buffer* buffer = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		ID3D11UnorderedAccessView* uav = nullptr;
	};

	struct NativeVertexBuffer
	{
		ID3D11Buffer* buffer = nullptr;
		uint32 stride = 0;
	};

	struct NativeIndexBuffer
	{
		ID3D11Buffer* buffer = nullptr;
		uint32 dxgi_format = 0;
	};

	struct NativeInstanceBuffer
	{
		ID3D11Buffer* buffer = nullptr;
	};

	struct NativeConstantBuffer
	{
		ID3D11Buffer* buffer = nullptr;
	};

	struct NativeBlendState
	{
		ID3D11BlendState* state = nullptr;
	};

	struct NativeRasterizerState
	{
		ID3D11RasterizerState* state = nullptr;
	};

	struct NativeDepthStencilState
	{
		ID3D11DepthStencilState* state = nullptr;
	};

	struct NativeInputSignature
	{
		ID3D11InputLayout* input_layout = nullptr;
	};

	struct NativeSampler
	{
		ID3D11SamplerState* sampler = nullptr;
	};

	struct NativeShader
	{
		ID3D11DeviceChild* shader = nullptr;
	};

	struct NativeProgram
	{
		NativeShader* vertex_shader = nullptr;
		NativeShader* geometry_shader = nullptr;
		NativeShader* pixel_shader = nullptr;
	};
}