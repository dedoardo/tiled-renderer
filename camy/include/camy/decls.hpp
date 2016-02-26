#pragma once

// camy
#include "base.hpp"

// All forward declarations to avoid bloating header files
using WindowHandle = void*;

using GraphicsAPIVersion = camy::u32;

struct ID3D11Device;
struct ID3D11DeviceContext;

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