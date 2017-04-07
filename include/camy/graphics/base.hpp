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

/*!
	Note: Resource descriptions are POD structures. They sometimes have methods
	attached to them only for convenience.
	States are grouped in a D3D-like way. OpenGL would just set each one manually 
	if no state objects are supported.
!*/
namespace camy
{
#define camy_declare_resource(name) struct name { Native##name native; name##Desc desc; };

	/*!
		Prerequisites validation. This is a measure to make sure a backend complies
		with the API defining all the required interfaces. If something fails to 
		compile here it's the backend's fault
	!*/
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

	using	  ContextID = decltype(kMaxConcurrentContexts);
	constexpr ContextID kInvalidContextID = (ContextID)-1;

	/*!
		Library-wide constants, behavior might depend on them. In case of tweaking code that
		relies on assumptions **should** fail to compile via static_assert and a message. 
		Having them allows for a good amount of compile time packing / optimizations.
	!*/
	constexpr rsize kMaxBindableVertexBuffers = 2u;
	constexpr rsize kMaxBindableRenderTargets = 2u;
	constexpr rsize kMaxParameterBlocks = 5u;
	constexpr rsize kMaxConstantBufferSize = 2 << 16;
	constexpr rsize kMaxBindableShaderResources = 2 << 7;
	constexpr rsize kMaxBindableConstantBuffers = 6;
	constexpr rsize kMaxPasses = 4;

	/*!
		Volume where the scene is projected. 
		left|top|right|bottom are in pixels
	!*/
	struct camy_api Viewport
	{
		uint16 left		= 0;
		uint16 top		= 0;
		uint16 right	= 0;
		uint16 bottom	= 0;
		float  near = 0.01f;
		float  far = 1.f;
	};

	/*!
		Wrapper for raw data. Unifies allocation / deallocation behind the
		courtains
	!*/
	struct camy_api Blob
	{
		void* data = nullptr;
		rsize byte_size = 0;

		bool contains_data()const;
		void allocate_data(const byte* in_data, rsize byte_size);
		void free_data();
	};

	/*!
		Ways in which a specific resource can be seen and thus bound to the GPU.
		For DX10+ users they map directly to views.
		OGL ?
	!*/
	enum camy_api GPUView
	{
		GPUView_None = 0,
		GPUView_ShaderResource = 1,
		GPUView_UnorderedAccess = 1 << 1,
		GPUView_RenderTarget = 1 << 2,
		GPUView_DepthStencil = 1 << 4,
	};

	enum class camy_api Usage
	{
		Static,
		Dynamic
	};

	//! Supported pixel formats
	enum class camy_api PixelFormat : uint8
	{
		Unknown,

		//! Compressed formats
		BC1Unorm, // DXT1
		BC3Unorm, // DXT3
		BC5Unorm, // DXT5

		//! Typeless formats, used for surfaces with different views (TODO: Reevaluate their use when the OpenGL backend is terminated)
		R8Typeless,
		R16Typeless,
		R32Typeless,
		R24G8Typeless,
		R24UnormX8Typeless,

		//! Single channel uncompressed
		R8Unorm,
		R16Unorm,
		R16Float,
		R32Float,

		//! Two channels uncompressed
		RG8Unorm,
		RG16Unorm,
		RG16Float,
		RG32Float,

		//! Four channels uncompressed
		RGBA8Unorm,
		RGBA16Float,
		RGBA32Float,

		//! Depth + Stencil formats
		D16Unorm,
		D32Float,
		D24UNorm_S8Uint
	};

	/*!
		Used at creation time to identify different elements of a image.
		can be:
		-> Element of  Surface2DArray
		-> Face of SurfaceCube
		-> Mipmaplevel of any face/element

		For more details on how data should be layed out see:
		https://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
	
		Note: Follows DDS/D3D conventions
		Width and Heights are not really needed as they can be derived from the surface
		description and their position in the array.
	!*/
	struct camy_api SubSurface
	{
		//! Pointer to the base of the data array
		const void* data = nullptr;

		//! Stride in bytes of each line(row).
		uint32 pitch = 0;
	};

	/*!
		Description of a Surface, currently only 2D textures, cubemaps and their 
		relative array versions are supported.
	!*/
	struct camy_api SurfaceDesc
	{
		//! Type of the resource in use
		enum class Type : uint8
		{
			Surface2D,
			Surface2DArray,
			SurfaceCube,
			SurfaceCubeArray,
		};

		//! Width in pixels of the surface
		uint16 width = 0;

		//! Height in pixels of the surface
		uint16 height = 0;

		//! Format of the actual resource
		PixelFormat pixel_format = PixelFormat::Unknown;

		//! Representation of the resource as seen from a GPUView_ShaderResourceView
		PixelFormat pixel_format_srv = PixelFormat::Unknown;

		//! Representation of the resource as seen from a GPUView_RenderTargetView
		PixelFormat pixel_format_rtv = PixelFormat::Unknown;

		//! Representation of the resource as seen from a GPUView_DepthStencilView
		PixelFormat pixel_format_dsv = PixelFormat::Unknown;

		//! Representation of the resource as seen from a GPUView_UnorderedAccessView
		PixelFormat pixel_format_uav = PixelFormat::Unknown;
		
		//! Views enabled for the cc
		uint8  gpu_views = GPUView_None;

		//! Number of miplevels of the specified resource
		//! Note: All subresources **HAVE TO HAVE** the same amount of miplevels
		uint8  mip_levels = 1;

		//! MSAA level of the surface, care about incompatibilities w/ shaders
		uint8  msaa_levels = 1;

		//! Number of surfaces **NOT** subresources. Makes sense only for Type::***Array types
		//! If the array_count doesn't match the type, then the array count is set to 1 and
		// a warning is posted.
		uint8  array_count = 1;

		//! Type of the resource
		Type   type = Type::Surface2D;

		//! Set this to dynamic if you plan to update this resource frequently (e.g. on a frame-by-frame basis)
		Usage usage = Usage::Static;
	};
	
	//! <Surface>
	camy_declare_resource(Surface);

	/*!
		Description of a Buffer, **not** a VertexBuffer, IndexBuffer or ConstantBuffer. 
		This simply holds data and binds it to the GPU as either a ShaderResourceView or
		UnorderedAccessView depending on the stage.
	!*/
	struct camy_api BufferDesc
	{
		//! Describes the data contained inside the buffer
		enum class Type
		{
			Default,	//! Buffer using standard scalar/vector/matrix types
			Structured, //! StructuredBuffer or SSBO
		};

		//! Type of the resource
		Type type = Type::Default;

		//! Number of elements 
		uint32 num_elements = 0;

		//! Size in bytes of every element
		uint32 element_size = 0;
	
		//! Set this to dynamic if you plan to update this resource frequently (e.g. on a frame-by-frame basis)
		Usage usage = Usage::Static;

		//! ShaderResource by default, true if you also want a uav
		bool is_uav = false;
	};

	//! <Buffer>
	camy_declare_resource(Buffer);

	//! Description of a vertex buffer, layout is detailed by <InputSignature>
	struct camy_api VertexBufferDesc
	{
		//! Number of elements
		uint32 num_elements = 0;

		//! Size in bytes of every element
		uint32 element_size = 0;

		//! Set this to dynamic if you plan to update this resource frequently (e.g. on a frame-by-frame basis)
		Usage usage;
	};

	//! <VertexBuffer>
	camy_declare_resource(VertexBuffer);

	//! Description of a index buffer, indices are either 
	struct camy_api IndexBufferDesc
	{
		//! Number of elements
		uint32 num_elements = 0;
		
		//! Set this to dynamic if you plan to update this resource frequently (e.g. on a frame-by-frame basis)
		Usage usage = Usage::Static;

		//! True if indices are 32-bits
		bool extended32 = false;
	};

	//! <IndexBuffer>
	camy_declare_resource(IndexBuffer);

	struct camy_api ConstantBufferDesc
	{
		uint32 size = 0;
	};

	//! <ConstantBuffer>
	camy_declare_resource(ConstantBuffer);

	//! Describes how blending should work, currently only presets are enabled
	//! Todo: Options
	struct camy_api BlendStateDesc
	{
		enum class Type
		{
			Opaque,
			Transparent,
			Additive
		};

		Type type = Type::Opaque;
	};

	camy_declare_resource(BlendState);

	//! Description of the rasterizer stage
	//! Todo: Options
	struct camy_api RasterizerStateDesc
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

	camy_declare_resource(RasterizerState);

	//! Description of a single shader stage. stages are grouped into a Program,
	//! just like D3D.
	struct camy_api ShaderDesc
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

	camy_declare_resource(Shader);

	//! Type of the specific shader binding
	enum class camy_api BindType : uint32
	{
		Sampler = 0,
		Surface,
		Buffer,
		ConstantBuffer,
		Constant, // Invalid as PipelineParameter
		LastValue
	};

	//! All possible information about a shader binding. Never create them yourself
	//! they are returned by the internal implementation. Setting them yourself can 
	//! be done, but it's not recommended.
	struct camy_api ShaderVariable
	{		
		// Could have used C bit-fields, but found a reason to try out binary literals
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

		//! valid by default
		ShaderVariable(bool valid = true) { data = valid ? 0x1 : 0x0; }

		//! Packed data
		uint32 data;

		//! Returns 1 if the shadervariable is valid. A valid bit allows for error checking
		//! at binding time.
		uint32 valid()const { return (data&kValidMask) >> kValidOff; }
		void valid(uint32 val) { data = (data & (~kValidMask)) | (val << kValidOff);  }

		//! Type
		uint32 type()const { return (data&kTypeMask) >> kTypeOff; }
		void type(uint32 val) { data = (data & (~kTypeMask)) | (val << kTypeOff); }

		//! Binding point
		uint32 slot()const { return (data&kSlotMask) >> kSlotOff; }
		void slot(uint32 val) { data = (data & (~kSlotMask)) | (val << kSlotOff); }

		//! ConstantBuffer -> byte size of the buffer
		//! Surface -> if array, number of elements
		//! Unused for the rest
		uint32 size()const { return (data&kSizeMask) >> kSizeOff; }
		void size(uint32 val) { data = (data & (~kSizeMask)) | (val << kSizeOff); }

		//! Pipeline stage this shadervariable refers to
		ShaderDesc::Type shader()const { return (ShaderDesc::Type)((data&kShaderMask) >> kShaderOff); }
		void shader(ShaderDesc::Type val) { data = ((uint32)data & (~kShaderMask)) | ((uint32)val << kShaderOff);  }

		//! is uav ?
		uint32 uav()const { return (data&kUavMask) >> kUavOff; }
		void uav(uint32 val) { data = (data & (~kUavMask) | (val << kUavOff)); }

		//! Checks if the current variable is valid
		bool is_valid()const {	return valid() == 1; }
		
		//! Returns a new invalid variable, used as return values for failing functions.
		static ShaderVariable make_invalid() { return ShaderVariable(false); };
	};

	//! Description of a single input element for the first vertex-processing
	//! stage (usually VS or GS).
	struct camy_api InputElement
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
	
	//! Description of the vertex buffer layout. Do not touch this directly,
	//! rely on reflection to generate them for you: Program::input_signature()
	//! TODO: Instancing is in the workings
	struct camy_api InputSignatureDesc
	{
		Blob bytecode;
		InputElement* elements;
		uint32 num_elements;
	};

	camy_declare_resource(InputSignature);

	//! Sampler object description
	//! Todo: Options
	struct camy_api SamplerDesc
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

	camy_declare_resource(Sampler);

	//! Depth stencil state description
	//! Todo: Options
	struct camy_api DepthStencilStateDesc
	{
		enum class DepthFunc
		{
			Less,
			LessEqual
		};

		DepthFunc depth_func = DepthFunc::Less;
	};

	camy_declare_resource(DepthStencilState);
}

//! Helper 
#if camy_enable_logging > 0
#include <ostream>
namespace camy
{
	std::ostream& operator<<(std::ostream& stream, const ShaderVariable& val);
}
#endif

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