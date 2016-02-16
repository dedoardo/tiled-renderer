// Header
#include <camy/resources.hpp>

#define NOMINMAX
#include <d3d11.h>
#include <dxgi.h>
#undef NOMINMAX

namespace camy
{
	namespace hidden
	{
		void Surface::dispose()
		{
			safe_release_com(texture_2d);
			safe_release_com(srv);
			safe_release_com(rtv);
			safe_release_com(dsv);
			safe_release_com(swap_chain);
		}

		void Buffer::dispose()
		{
			safe_release_com(buffer);
			safe_release_com(srv);
		}

		void VertexBuffer::dispose()
		{
			safe_release_com(buffer);
		}

		void IndexBuffer::dispose()
		{
			safe_release_com(buffer);
		}

		void ConstantBuffer::dispose()
		{
			safe_release_com(buffer);
		}

		void BlendState::dispose()
		{
			safe_release_com(state);
		}

		void RasterizerState::dispose()
		{
			safe_release_com(state);
		}

		void InputSignature::dispose()
		{
			safe_release_com(input_layout);
		}

		void Sampler::dispose()
		{
			safe_release_com(sampler);
		}

		void Shader::dispose()
		{
			safe_release_com(shader);
		}
	}

	Surface::Format Surface::translate(u32 format)
	{
		DXGI_FORMAT dxgi_format{ static_cast<DXGI_FORMAT>(format) };
		switch (dxgi_format)
		{
		case DXGI_FORMAT_BC3_UNORM:
			return Format::BC3Unorm;

		default:
			return Format::Unknown;
		}
	}
}