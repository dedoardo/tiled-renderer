#pragma once

// camy
#include "camy_base.hpp"
#include "com_utils.hpp"

namespace camy
{
	namespace hidden
	{
		/*
			The reason why there is no inheritance or virtual function for disposing is that this code is not going to change
			and adding all this oopness doesn't really make sense in the end, no one will benefit from it
		*/
		struct Surface final
		{
			ID3D11Texture2D*			texture_2d{ nullptr };
			ID3D11ShaderResourceView*	srv{ nullptr };
			ID3D11RenderTargetView*		rtv{ nullptr };
			ID3D11DepthStencilView*		dsv{ nullptr };
			ID3D11UnorderedAccessView*  uav{ nullptr };
			IDXGISwapChain*				swap_chain{ nullptr };
			WindowHandle				window_handle{ nullptr };

			void dispose();
		};

		struct Buffer final
		{
			ID3D11Buffer* buffer{ nullptr };
			ID3D11ShaderResourceView* srv{ nullptr };
			ID3D11UnorderedAccessView* uav{ nullptr };

			void dispose();
		};

		struct VertexBuffer final
		{
			ID3D11Buffer* buffer{ nullptr };

			void dispose();
		};

		struct IndexBuffer final
		{
			ID3D11Buffer* buffer{ nullptr };

			void dispose();
		};

		struct ConstantBuffer final
		{
			ID3D11Buffer* buffer{ nullptr };

			void dispose();
		};

		struct BlendState final
		{
			ID3D11BlendState* state{ nullptr };
	
			void dispose();
		};

		struct RasterizerState final
		{
			ID3D11RasterizerState* state{ nullptr };

			void dispose();
		};

		struct InputSignature final
		{
			ID3D11InputLayout* input_layout{ nullptr };

			void dispose();
		};

		struct Sampler final
		{
			ID3D11SamplerState* sampler{ nullptr };
		
			void dispose();
		};

		struct Shader final
		{
			ID3D11DeviceChild* shader{ nullptr };

			void dispose();
		};

		struct DepthStencilState final
		{
			ID3D11DepthStencilState* state{ nullptr };

			void dispose();
		};

	}

	// Used when initializing surfaces with initial data 
	// and multiple faces / arraysizes
	struct SubSurface final
	{
		u32 pitch{ 0u };
		u32 width{ 0u };
		u32 height{ 0u };
		u32 size{ 0u };
		void* data{ 0u };
	};

	enum class PrimitiveTopology : u16
	{
		PointList,
		TriangleList,
		TriangleStrip,
		LineList,
		LineStrip
	};

	struct Viewport
	{
		float left, right;
		float top, bottom;
	};

	enum class BindType : u32 
	{
		Sampler = 0,
		Surface = 1,
		Buffer  = 2,
		ConstantBuffer,
		LastValue
	};

	struct PipelineResource
	{
		PipelineResource(BindType type) : type{ type } { } 

		BindType type;
	};

	struct Surface : PipelineResource
	{
		Surface() : PipelineResource(BindType::Surface) { };

		enum class Format
		{
			Unknown,
			// Compressed formats
			BC1Unorm,
			BC3Unorm,

			// Typeless formats, used for surfaces in different contexts ( different views )
			R16Typeless,

			R32Typeless,
			R24G8Typeless,
			R24UnormX8Typeless,

			// Uncompressed formats
			RGBA8Unorm,
			RGBA16Float,
			RGBA32Float,
			R16Float,
			R16Unorm,
			R32Float,

			// Depth formats
			D16Unorm,
			D32Float,
			D24UNorm_S8Uint,
		};

		static Format translate(u32 format);

		struct Description
		{
			Format format_srv{ Format::Unknown };
			Format format_rtv{ Format::Unknown };
			Format format_dsv{ Format::Unknown };
			Format format_uav{ Format::Unknown };
			u32		width{ 0u };
			u32		height{ 0u };
			u8		msaa_level{ 1u };
			bool	is_dynamic{ false };
			Format format{ Format::Unknown };
		};

		Description description;

		hidden::Surface hidden;
	};

	struct Buffer : PipelineResource
	{
		Buffer() : PipelineResource(BindType::Buffer) { } 

		enum class Type
		{
			Structured
			// Uav and more to come
		};

		Type type;
		u32 element_size;
		u32 element_count;
		bool is_dynamic;
		hidden::Buffer hidden;
	};

	struct VertexBuffer
	{
		u32  element_size;
		u32  element_count;
		bool is_dynamic;
		
		hidden::VertexBuffer hidden;
	};

	struct IndexBuffer
	{
		enum class Type
		{
			U16,
			U32
		};

		Type index_type;
		u32  element_count;

		hidden::IndexBuffer hidden;
	};

	struct ConstantBuffer : PipelineResource
	{
		ConstantBuffer() : PipelineResource(BindType::ConstantBuffer) { } 

		u32 size;
		hidden::ConstantBuffer hidden;
	};

	/*
		Todo: Render target indipendent blending
	*/
	struct BlendState
	{
		enum class Mode
		{
			Opaque,
			Transparent,
			Additive
		};

		Mode mode;

		hidden::BlendState hidden;
	};

	struct RasterizerState
	{
		enum class Cull
		{
			Front, 
			Back,
			None
		};

		enum class Fill
		{
			Solid, 
			Wireframe
		};

		Cull cull;
		Fill fill;

		hidden::RasterizerState hidden;
	};

	struct InputSignature
	{
		hidden::InputSignature hidden;
	};

	struct Sampler : PipelineResource
	{
		Sampler() : PipelineResource(BindType::Sampler) { } 

		enum class Filter
		{
			Point,
			Linear,
			Anisotropic
		};

		enum class Comparison
		{
			Never,
			Less,
			LessEqual
		};

		enum class Address
		{
			Clamp,
			Wrap,
			Mirror
		};

		Filter	filter;
		Comparison comparison;
		Address address;

		hidden::Sampler hidden;
	};

	struct DepthStencilState
	{
		hidden::DepthStencilState hidden;
	};
}