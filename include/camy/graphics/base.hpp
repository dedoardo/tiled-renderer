/* base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/#pragma once

// camy
#include <camy/graphics/platform.hpp>
#include <camy/core/memory/static_string.hpp>
#include <camy/core/memory/vector.hpp>
#include <camy/core/memory/auto_ptr.hpp>

/*
	In order to avoid interfaces for different backends all that is required is 
	this header for each backend.
	After the header is included checks are made to ensure that all the required
	types have been declared.
*/
#if defined(camy_backend_d3d11)
#	include "d3d11/d3d11_graphics_base.hpp"
#elif defined(camy_backend_opengl4)
#	include "opengl4/opengl4_graphics_base.hpp"
#else
#	error No supported backend has been specified
#endif

#define _camy_is_defined(type) static_assert(sizeof(type) >= 0, "No " #type " defined for graphics_base interface");

// TODO: this should not happen
#if defined(near)
#undef near
#endif
#if defined(far)
#undef far
#endif

// Ensuring interface requirements are met
namespace camy
{
	_camy_is_defined(NativeSurface);
	_camy_is_defined(NativeBuffer);
	_camy_is_defined(NativeVertexBuffer);
	_camy_is_defined(NativeIndexBuffer);
	_camy_is_defined(NativeInstanceBuffer);
	_camy_is_defined(NativeConstantBuffer);
	_camy_is_defined(NativeBlendState);
	_camy_is_defined(NativeRasterizerState);
	_camy_is_defined(NativeDepthStencilState);
	_camy_is_defined(NativeInputSignature);
	_camy_is_defined(NativeSampler);
	_camy_is_defined(NativeShader);
	_camy_is_defined(kMaxConcurrentContexts);

	using ContextID = decltype(kMaxConcurrentContexts);
	constexpr ContextID kInvalidContextID = (ContextID)-1;

	/*
		Top consic: Constants
			Alltants ( resource limits ) for the engine, they can be tweaked
			if more slots are needed. Tweaking them might require adding some
			default values around the code. They are not 
	*/
	constexpr rsize kMaxBindableVertexBuffers = 2u;
	constexpr rsize kMaxBindableRenderTargets = 2u;
    constexpr rsize kMaxParameterBlocks = 5u;
	constexpr rsize kMaxConstantBufferSize = 2 << 16;
	constexpr rsize kMaxBindableShaderResources = 2 << 7;
	constexpr rsize kMaxBindableConstantBuffers = 6;
	constexpr rsize kMaxInputElements = 8; // TODO: Might remove this, just here for simplicty atm
    constexpr rsize kMaxPasses = 4;

	enum GPUView
	{
		GPUView_None		    = 0,
		GPUView_ShaderResource  = 1,
		GPUView_UnorderedAccess = 1 << 1,
		GPUView_RenderTarget    = 1 << 2,
		GPUView_DepthStencil    = 1 << 4,
	};

	struct Viewport
	{
		uint16 left		= 0;
		uint16 top		= 0;
		uint16 right	= 0;
		uint16 bottom	= 0;
        float  near = 0.01f;
        float  far = 1.f;
	};

	enum class PixelFormat : uint16
	{
		Unknown,
		// Compressed formats
		BC1Unorm,
		BC3Unorm,

		// Typeless formats, used for surfaces in different contexts ( different views )
        // This is very D3D-like will be reevaluated when the opengl backend is implemented (TODO)
        R8Typeless,
		R16Typeless,
		R32Typeless,
		R24G8Typeless,
		R24UnormX8Typeless,

        // -> Single channel uncompressed
        R8Unorm,
        R16Unorm,
		R16Float,
		R32Float,

        // -> Two channels uncompressed
        RG8Unorm,
        RG16Unorm,
        RG16Float,
        RG32Float,

        // -> Four channels uncompressed
        RGBA8Unorm,
        RGBA16Float,
        RGBA32Float,

		// Depth formats
		D16Unorm,
		D32Float,
		D24UNorm_S8Uint,
	};

    /**
        Wrapper for raw data. Unifies allocation / deallocation behind the
        courtains
    */
    struct Blob
    {
        void* data = nullptr;
        rsize byte_size = 0;

		bool contains_data()const;
        void allocate_data(const byte* in_data, rsize byte_size);
        void free_data();
    };

	struct SubSurface
	{
        const void* data = nullptr;
        uint32 pitch = 0;
	};

	// TODO: Could be packed in a better way
	struct SurfaceDesc
	{
		enum class Type : uint8
		{
			Surface2D,
			Surface2DArray,
			SurfaceCube,
			SurfaceCubeArray,
		};

        PixelFormat pixel_format = PixelFormat::Unknown;
        PixelFormat pixel_format_srv = PixelFormat::Unknown;
        PixelFormat pixel_format_rtv = PixelFormat::Unknown;
        PixelFormat pixel_format_dsv = PixelFormat::Unknown;
        PixelFormat pixel_format_uav = PixelFormat::Unknown;
        uint16 width = 0;
        uint16 height = 0;
        uint8  gpu_views = GPUView_None;
        uint8  mip_levels = 1;
        uint8  msaa_levels = 1;
        uint8  surface_count = 1;
        Type   type = Type::Surface2D;
        bool   is_dynamic = false;

		// The number of subsurfaces depends on the type(1 or 6 for cubemaps) * surface_count * miplevels.
		// ordered as if it was a DDS texture:
		//  > First element
		//	> First element Miplevels
		//  > Second element 
		//  > Second element Miplevels
		// Element can be either a cubemap face or a single 
        Vector<SubSurface> initial_subsurface_data;
	};

	struct Surface
	{
		NativeSurface native;
		SurfaceDesc desc;
	};

	struct BufferDesc
	{
		enum class Type
		{
			Structured,
			// Raw..
		};

		Type type;
		uint32 num_elements;
		uint32 element_size;
		uint32 gpu_views;
	};

	struct Buffer
	{
		NativeBuffer native;
		BufferDesc   desc;
	};

	struct VertexBufferDesc
	{
		uint32 num_elements;
		uint32 element_size;
		bool   is_dynamic;
		const void* initial_data;
	};

	struct VertexBuffer
	{
		NativeVertexBuffer native;
		VertexBufferDesc desc;
	};

	struct IndexBufferDesc
	{
		uint32 num_elements;
		uint32 element_size;
		bool   is_dynamic;
		const void* initial_data;
	};

	struct IndexBuffer
	{
		NativeIndexBuffer native;
		IndexBufferDesc desc;
	};

	struct ConstantBufferDesc
	{
		uint32 size;
	};

	struct ConstantBuffer
	{
		NativeConstantBuffer native;
		ConstantBufferDesc desc;
	};

	struct BlendStateDesc
	{
		enum class Type
		{
			Opaque,
			Transparent,
			Additive
		};

		Type type;
	};

	struct BlendState
	{
		NativeBlendState native;
		BlendStateDesc desc;
	};

	struct RasterizerStateDesc
	{
		enum class Fill
		{
			Solid,
			Wireframe
		};

		enum class Cull
		{
			Back,
			Front,
			None
		};

		Fill fill = Fill::Solid;
		Cull cull = Cull::Back;
		uint32 depth_bias = 0;
		float  depth_bias_clamp = 0.f;
		float  slope_scaled_depth_bias = 0.f;
	};

	struct RasterizerState
	{
		NativeRasterizerState native;
		RasterizerStateDesc desc;
	};

	struct ShaderDesc
	{
		enum class Type
		{
			Vertex,
			Geometry,
			Pixel,
			Compute,
            Count
		};
		Type type;
		Blob bytecode;
	};

	struct Shader
	{
		ShaderDesc desc;
		NativeShader native;
	};

	enum class BindType : uint32
	{
		Sampler = 0,
		Surface,
		Buffer,
		ConstantBuffer,
        Constant, // Invalid as PipelineParameter
        LastValue
	};

	struct camy_api ShaderVariable
	{		
		// We don't like C bitfields and found a reason to try binary literal
		// Could also shift then mask
		static constexpr uint32 kValidMask =  0b00000000000000000000000000000001;
		static constexpr uint32 kValidOff = 0;
		static constexpr uint32 kTypeMask =   0b00000000000000000000000000000110;
		static constexpr uint32 kTypeOff = 1;
		static constexpr uint32 kSlotMask =   0b00000000000000000000001111111000;
		static constexpr uint32 kSlotOff = 3;
		static constexpr uint32 kSizeMask =   0b00000011111111111111110000000000;
		static constexpr uint32 kSizeOff = 10;
		static constexpr uint32 kShaderMask = 0b00011100000000000000000000000000;
		static constexpr uint32 kShaderOff = 26;
		static constexpr uint32 kUavMask =    0b00100000000000000000000000000000;
		static constexpr uint32 kUavOff = 29;
		static constexpr uint32 kPaddingMask = 0b00000000000000000000000000000000;

		// Packed data
		uint32 data = 0x1; // Making it valid by default

		uint32 valid()const { return (data&kValidMask) >> kValidOff; }
		void valid(uint32 val) { data = (data & (~kValidMask)) | (val << kValidOff);  }

		uint32 type()const { return (data&kTypeMask) >> kTypeOff; }
		void type(uint32 val) { data = (data & (~kTypeMask)) | (val << kTypeOff); }

		uint32 slot()const { return (data&kSlotMask) >> kSlotOff; }
		void slot(uint32 val) { data = (data & (~kSlotMask)) | (val << kSlotOff); }

		uint32 size()const { return (data&kSizeMask) >> kSizeOff; }
		void size(uint32 val) { data = (data & (~kSizeMask)) | (val << kSizeOff); }

		ShaderDesc::Type shader()const { return (ShaderDesc::Type)((data&kShaderMask) >> kShaderOff); }
		void shader(ShaderDesc::Type val) { data = ((uint32)data & (~kShaderMask)) | ((uint32)val << kShaderOff);  }

		uint32 uav()const { return (data&kUavMask) >> kUavOff; }
		void uav(uint32 val) { data = (data & (~kUavMask) | (val << kUavOff)); }

		bool is_valid()const {	return valid() == 0; }

		static ShaderVariable invalid() { ShaderVariable sv; sv.valid(1); return sv; };
	};
#if !defined(camy_enable_layers_validation)
	static_assert(sizeof(ShaderVariable) == sizeof(decltype(ShaderVariable::data)), "ShaderVariable not correctly packed");
#endif

	struct InputElement
	{
		using Name = StaticString<25>;

		enum class Type : uint8
		{
			SInt,
			SInt2,
			SInt3,
			SInt4,
			UInt,
			UInt2,
			UInt3,
			UInt4,
			Float,
			Float2,
			Float3,
			Float4
		};

		Name name;
		uint8 semantic_idx;
		uint8 slot;
		bool is_instanced;
		Type type;
	};

	struct InputSignatureDesc
	{
		Blob bytecode;
		InputElement* elements;
		uint32 num_elements;
	};

	struct InputSignature
	{
		NativeInputSignature native;
		InputSignatureDesc desc;
	};

	struct SamplerDesc
	{
		enum class Address
		{
			Clamp,
			Wrap,
			Mirror
		};

		enum class Filter
		{
			Point,
			Linear,
			Anisotropic
		};

		enum class Comparison
		{
			Less,
			LessEqual,
			Never
		};

		Address address = Address::Wrap;
		Filter filter = Filter::Linear;
		Comparison comparison = Comparison::Never;
	};

	struct Sampler
	{
		NativeSampler native;
		SamplerDesc desc;
	};

	struct DepthStencilStateDesc
	{
		enum class DepthFunc
		{
			Less,
			LessEqual
		};

		DepthFunc depth_func;
	};

	struct DepthStencilState
	{
		NativeDepthStencilState native;
		DepthStencilStateDesc desc;
	};
}

#if defined(camy_backend_d3d11)
#	include "d3d11/d3d11_impl_base.hpp"
#elif defined(camy_backend_opengl4)
#	include "opengl4/opengl4_impl_base.hpp"
#else
#	error No supported backend has been specified
#endif

namespace camy
{
	_camy_is_defined(RenderContextData);
	_camy_is_defined(CommandListData);
}