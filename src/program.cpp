/* program.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/program.hpp>

// camy
#include <camy/graphics.hpp>
#include <camy/render_context.hpp>

namespace camy
{
	Program::Program() { } 

	Program::~Program()
	{
		unload();
	}

	bool Program::load_from_text(const ProgramDesc& desc, const char8* name)
	{
		unload();

		ProgramDesc bytecode_desc;
		if (desc.vertex_src.contains_data())
			compile_stage(ShaderDesc::Type::Vertex, desc.vertex_opts, desc.vertex_src, bytecode_desc.vertex_src);
		if (desc.pixel_src.contains_data())
			compile_stage(ShaderDesc::Type::Pixel, desc.pixel_opts, desc.pixel_src, bytecode_desc.pixel_src);

		return load_from_bytecode(bytecode_desc, name);
	}

	bool Program::load_from_bytecode(const ProgramDesc& desc, const char8* name)
	{
		if (desc.vertex_src.contains_data())
		{
			ShaderDesc shader_desc;
			shader_desc.bytecode = desc.vertex_src;
			shader_desc.type = ShaderDesc::Type::Vertex;

			StaticString<60> shader_name = name;
			shader_name.append("_vertex");

			HResource shader = API::rc().create_shader(shader_desc, shader_name.str());
			if (shader.is_invalid())
			{
				CL_ERR("Failed to create VertexShader: ", name);
				unload();
				return false;
			}
			m_shaders[(rsize)shader_desc.type] = shader;
		}

		if (desc.pixel_src.contains_data())
		{
			ShaderDesc shader_desc;
			shader_desc.bytecode = desc.pixel_src;
			shader_desc.type = ShaderDesc::Type::Pixel;

			StaticString<60> shader_name = name;
			shader_name.append("_pixel");

			HResource shader = API::rc().create_shader(shader_desc, shader_name.str());
			if (shader.is_invalid())
			{
				CL_ERR("Failed to create PixelShader", name);
				unload();
				return false;
			}
			m_shaders[(rsize)shader_desc.type] = shader;
		}

		if (reflect(ShaderDesc::Type::Vertex, desc.vertex_src, name) == false ||
			reflect(ShaderDesc::Type::Pixel, desc.pixel_src, name) == false)
		{
			CL_ERR("Failed to reflect shaders");
			unload();
			return false;
		}

		return true;
	}

	bool Program::compile_stage(ShaderDesc::Type type, const CompileOpts& opts, const Blob& data, Blob& data_out)
	{
		return impl_compile_stage(type, opts, data, data_out);
	}

	void Program::unload()
	{
		API::rc().destroy_input_signature(m_input_signature);
		for (rsize i = 0; i < (rsize)ShaderDesc::Type::Count; ++i)
		{
			m_variables[i].clear();
			m_bindings[i].clear();
			API::rc().destroy_shader(m_shaders[i]);
		}
	}

	ShaderVariable Program::var(ShaderDesc::Type type, const char8* name)
	{
		uint64* idx = m_variables[(rsize)type][name];
		if (idx == nullptr || *idx >= m_bindings[(rsize)type].count())
		{
			CL_ERR("Failed to find var: ", name);
			return ShaderVariable::make_invalid();
		}

		return m_bindings[(rsize)type][(rsize)*idx].var;
	}

	HResource Program::input_signature()
	{
		return m_input_signature;
	}

	HResource Program::shader(ShaderDesc::Type type)
	{
		return m_shaders[(rsize)type];
	}
	
	HResource Program::vs()
	{
		return shader(ShaderDesc::Type::Vertex);
	}

	HResource Program::ps()
	{
		return shader(ShaderDesc::Type::Pixel);
	}

	ShaderVariable Program::vertex_var(const char8 * name)
	{
		return var(ShaderDesc::Type::Vertex, name);
	}

	ShaderVariable Program::pixel_var(const char8 * name)
	{
		return var(ShaderDesc::Type::Pixel, name);
	}
}