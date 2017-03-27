/* program.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/graphics/base.hpp>
#include <camy/core/memory/int2int_map.hpp>
#include <camy/core/memory/vector.hpp>
#include <camy/core/memory/static_string.hpp>
#include <camy/graphics/resource_manager.hpp>

namespace camy
{
	struct CompileOpts
	{
		struct Def
		{
			const char8* name;
			const char8* val;
		};

		Def* defs = nullptr;
		rsize num_defs = 0;
		const char8* base_path = nullptr;
		const char8* entry_point = "main";
	};

	struct ProgramDesc
	{
		Blob vertex_src;
		CompileOpts vertex_opts;
		Blob pixel_src;
		CompileOpts pixel_opts;
	};

	class Program final
	{
	public:
		//! Name of shader variables, capped to 64 bytes
		using VarName = StaticString<64>;

		//! Possible binding to pipeline
		struct Binding
		{
			//! String name
			VarName name;

			//! Shader variable
			ShaderVariable var;
		};

	public:
		Program();
		~Program();

		bool load_from_text(const ProgramDesc& desc, const char8* name = nullptr);
		bool load_from_bytecode(const ProgramDesc& desc, const char8* name = nullptr);
		bool compile_stage(ShaderDesc::Type type, const CompileOpts& opts, const Blob& desc, Blob& data_out);
		void unload();

		ShaderVariable var(ShaderDesc::Type type, const char8* name);
		HResource input_signature();

		HResource shader(ShaderDesc::Type type);

		// Helpers
		HResource vs();
		HResource ps();
		ShaderVariable vertex_var(const char8* name);
		ShaderVariable pixel_var(const char8* name);
		template <typename NamedCBuffer> ShaderVariable vertex_var() { return vertex_var(NamedCBuffer::_cbuffer_name); }
		template <typename NamedCBuffer> ShaderVariable pixel_var() { return pixel_var(NamedCBuffer::_cbuffer_name); }

	private:
		//! Platform dependent implementation
		bool impl_compile_stage(ShaderDesc::Type type, const CompileOpts&, const Blob& desc, Blob& data_out);
		bool reflect(ShaderDesc::Type type, const Blob& data, const char8* name);

		HResource m_input_signature;
		HResource m_shaders[(rsize)ShaderDesc::Type::Count];
		
		Int2IntMap m_variables[(rsize)ShaderDesc::Type::Count];
		Vector<Binding> m_bindings[(rsize)ShaderDesc::Type::Count];
	};
}