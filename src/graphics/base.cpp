/* base.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/base.hpp>

namespace camy
{
	bool Blob::contains_data() const
	{
		return byte_size > 0 && data != nullptr;
	}

	void Blob::allocate_data(const byte * in_data, rsize in_byte_size)
	{
		free_data();
		data = allocate(camy_loc, in_byte_size);
		if (in_data != nullptr)
			std::memcpy(data, in_data, in_byte_size);
		byte_size = in_byte_size;

	}

	void Blob::free_data()
	{
		deallocate(data);
		byte_size = 0;
	}
}

#if camy_enable_logging > 0
namespace camy
{
	const char8* bindtype_to_str(BindType bindtype)
	{
		switch (bindtype)
		{
		case BindType::Buffer: return "Buffer";
		case BindType::Constant: return "Constant";
		case BindType::ConstantBuffer: return "ConstantBuffer";
		case BindType::Sampler: return "SamplerState";
		case BindType::Surface: return "Surface";
		default: return "<Unknown>";
		}
	}

	const char8* stage_to_str(ShaderDesc::Type type)
	{
		switch (type)
		{
		case ShaderDesc::Type::Vertex: return "Vertex";
		case ShaderDesc::Type::Pixel: return "Pixel";
		case ShaderDesc::Type::Geometry: return "Geometry";
		case ShaderDesc::Type::Compute: return "Compute";
		}
	}

	std::ostream& operator<<(std::ostream& stream, const ShaderVariable& val)
	{
		if (!val.is_valid())
		{
			stream << "Invalid ShaderVariable";
			return stream;
		}

		stream << "ShaderVariable <" <<
			"BindType=" << bindtype_to_str((BindType)val.type()) << ", " <<
			"Slot=" << val.slot() << ", " <<
			"Stage=" << stage_to_str((ShaderDesc::Type)val.shader()) << ">";

		return stream;
	}
}
#endif