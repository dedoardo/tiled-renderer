/* program.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/program.hpp>

#if defined(camy_os_windows) && defined(camy_backend_opengl4)

// camy
#include <camy/graphics/platform.hpp>
#include <camy/graphics/render_context.hpp>

namespace camy
{
	const char8 _camy_default_binding[] = "$_camy_binding_$";

	ShaderStage::ShaderStage() :
		m_shader_handle(kInvalidHResource),
		m_input_signature_handle(kInvalidHResource)
	{

	}

	ShaderStage::~ShaderStage()
	{
		destroy();
	}

	bool ShaderStage::load_from_bytecode(Type type, const Blob& data, const char8* name)
	{
		camy_assert(data.data != nullptr);

		// Releasing previous one
		destroy();
		
		// Creating shadder
		ShaderDesc shader_desc;
		shader_desc.bytecode = data;
		shader_desc.type = type;
		m_shader_handle = API::rc().create_shader(shader_desc, name);
		if (m_shader_handle == kInvalidHResource)
		{
			camy_error("Failed to create ShaderStage: ", name);
			destroy();
			return false;
		}

		m_type = type;

		// Reflecting
		_reflect_bytecode(data, name);
		
		camy_info("Successfully created ShaderStage: ", name);
		return true;
	}


	bool ShaderStage::load_from_text(const char8* text, const CompileOptions& opts, const char8* name)
	{
		Blob bytecode = compile_to_bytecode(text, opts);
		if (!bytecode.contains_data())
		{
			camy_error("Failed to load from text: ", name);
			return false;
		}

		bool res = load_from_bytecode(opts.type, bytecode, name);
		bytecode.free_data();
		return res;
	}

	Blob ShaderStage::compile_to_bytecode(const char8* text, const CompileOptions& opts)
	{
		Blob blob;
		blob.allocate_data((const byte*)text, ::camy::strlen(text) + 1);
		return blob;
	}

	const ShaderStage::Binding* ShaderStage::find(const char8* name)
	{
		uint64* idx = m_variables[name];
		if (idx == nullptr || *idx >= m_bindings.count())
		{
			camy_error("Failed to find shader variable: ", name);
			return nullptr;
		}
		return &m_bindings[(rsize)*idx];
	}

	const ShaderStage::Binding* ShaderStage::find_by_index(rsize idx)
	{
		VarName name = _camy_default_binding;
		name.append(idx);
		return find(name.str());
	}

	void ShaderStage::destroy()
	{
	}

	const ShaderStage::Binding* ShaderStage::get_bindings()const
	{
		return m_bindings.data();
	}

	rsize ShaderStage::get_bindings_count()const
	{
		return m_bindings.count();
	}

	ShaderStage::Type ShaderStage::type()const
	{
		return m_type;
	}

	HResource ShaderStage::get_shader_handle() const
	{
		return m_shader_handle;
	}

	HResource ShaderStage::get_input_signature_handle() const
	{
		return m_input_signature_handle;
	}

	bool ShaderStage::_reflect_bytecode(const Blob& data, const char8* name)
	{
		// Clearing
		m_variables.clear();

		Shader& shader = API::rc().get_shader(m_shader_handle);

		for (rsize i = 0; i < GL_ACTIVE_ATTRIBUTES; ++i)
		{
			GLint size;
			GLenum type;
			glGetActiveAttrib(shader.native.shader, i, 0, nullptr, &size, &type, nullptr);
		}

		return false;
	}
}
#endif