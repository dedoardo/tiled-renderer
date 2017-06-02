/* program.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core.hpp>
#include <camy/program.hpp>

#if defined(CAMY_BACKEND_OPENGL4)

// camy
#include <camy/render_context.hpp>

namespace camy
{
	bool Program::impl_compile_stage(ShaderDesc::Type type, const CompileOpts& opts, const Blob& text, Blob& data_out)
	{
		Vector<char8> buffer;
		rsize cur_off = 0;
		const char8* version_tag = "#version 450\n";
		const rsize version_tag_len = API::strlen(version_tag);
		buffer.resize(version_tag_len);
		memcpy(buffer.data(), version_tag, version_tag_len);
		cur_off += version_tag_len;

		StaticString<255> entry_point = "#define ";
		entry_point.append(opts.entry_point);
		entry_point.append(" main\n");
		buffer.resize(buffer.count() + entry_point.len());
		memcpy(buffer.data() + cur_off, entry_point.str(), entry_point.len());
		cur_off += entry_point.len();

		for (rsize i = 0; i < opts.num_defs; ++i)
		{
			StaticString<255> line;
			line.append("#define ");
			line.append(opts.defs[i].name);
			line.append(" ");
			line.append(opts.defs[i].val);
			buffer.resize(buffer.count() + line.len());
			memcpy(buffer.data() + cur_off, line.str(), line.len());
			buffer.append('\n');
			cur_off += line.len() + 1;
		}

		data_out.allocate(nullptr, buffer.count() + text.size);
		memcpy(data_out.data, buffer.data(), buffer.count());
		memcpy((byte*)data_out.data + buffer.count(), text.data, text.size);
		return true;
	}

	InputElement::Type opengl_to_camy(GLint type)
	{
		switch (type)
		{
		case GL_FLOAT:
			return InputElement::Type::Float;
		case GL_FLOAT_VEC2:
			return InputElement::Type::Float2;
		case GL_FLOAT_VEC3:
			return InputElement::Type::Float3;
		case GL_FLOAT_VEC4:
			return InputElement::Type::Float4;
		case GL_INT:
			return InputElement::Type::SInt;
		case GL_INT_VEC2:
			return InputElement::Type::SInt2;
		case GL_INT_VEC3:
			return InputElement::Type::SInt3;
		case GL_INT_VEC4:
			return InputElement::Type::SInt4;
		case GL_UNSIGNED_INT:
			return InputElement::Type::UInt;
		case GL_UNSIGNED_INT_VEC2:
			return InputElement::Type::UInt2;
		case GL_UNSIGNED_INT_VEC3:
			return InputElement::Type::UInt3;
		case GL_UNSIGNED_INT_VEC4:
			return InputElement::Type::UInt4;
		default:
			return InputElement::Type::Unsupported;
		}
	}

	bool Program::reflect(ShaderDesc::Type shader_type, const Blob& data, const char8* name)
	{
		GLuint program = API::rc().get_shader(m_shaders[(rsize)shader_type]).native.shader;
		rsize type = (rsize)shader_type;

		GLint active_ubs;
		glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &active_ubs);
		
		GLint max_ub_name_len;
		glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NAME_LENGTH, &max_ub_name_len);
		
		ArrayAutoPtr<char8> ub_name = (char8*)API::allocate(CAMY_UALLOC(sizeof(char8) * max_ub_name_len));
		
		// Constant buffers
		for (GLint i = 0; i < active_ubs; ++i)
		{
			glGetProgramResourceName(program, GL_UNIFORM_BLOCK, i, max_ub_name_len, nullptr, ub_name);
			GLint idx = glGetUniformBlockIndex(program, ub_name);

			GLenum size_prop = GL_BUFFER_DATA_SIZE;
			GLint ub_size;
			glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, i, 1, &size_prop, 1, nullptr, &ub_size);

			Binding binding;
			binding.name = (char8*)ub_name;
			binding.var.type((uint32)BindType::ConstantBuffer);
			binding.var.slot((uint32)glGetUniformBlockIndex(program, ub_name));
			binding.var.size((uint32)ub_size);
			binding.var.shader(shader_type);
			binding.var.uav(0);

			m_variables[type].insert(ub_name, m_bindings[type].count());
			m_bindings[type].append(binding);
		}

		// Resources

		// Input layout
		if (shader_type == ShaderDesc::Type::Vertex)
		{
			Vector<InputElement> elements;
			GLint num_inputs;
			glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &num_inputs);

			for (GLint i = 0; i < num_inputs; ++i)
			{
				GLenum input_props[] = { GL_TYPE, GL_LOCATION };
				GLint input_vals[2];
				glGetProgramResourceiv(program, GL_PROGRAM_INPUT, i, ARRAYSIZE(input_props), input_props, ARRAYSIZE(input_props), nullptr, input_vals);
			
				InputElement next;
				next.name = "";
				next.semantic_idx = input_vals[1];
				next.type = opengl_to_camy(input_vals[0]);

				elements.append(next);
			}

			InputSignatureDesc isd;
			isd.elements = API::tallocate_array<InputElement>(CAMY_UALLOC(elements.count()));
			memcpy(isd.elements, elements.data(), sizeof(InputElement) * elements.count());
			isd.num_elements = elements.count();

			m_input_signature = API::rc().create_input_signature(isd, name);
			if (isd.num_elements > 0 && m_input_signature.is_invalid())
			{
				CL_ERR("Failed to create InputSignature: ", name);
				return false;
			}
		}

		return true;
	}
}
#endif 