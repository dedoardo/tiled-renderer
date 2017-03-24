/* shader_stage.hpp
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
    // Extended version of shader that allows for compilation and reflection
    // also generates inputlayout if vertex shader. 
    class camy_api ShaderStage final
	{
	public:
		using Type = ShaderDesc::Type;

        using VarName = StaticString<60>;
    
        /**
            Represents the member of a constant buffer
        */
        struct Constant
        {
            VarName name;
            uint16 offset;  
            uint16 size;
        };

        struct StructuredBinding
        {

        };
        
        struct Binding
        {
            ShaderVariable var;

            Vector<VarName> constants; // If ConstantBuffer
            Constant constant; // If constant
        };

        /**
            Compile options.  
        */
        struct CompileOptions
        {
            struct Macro
            {
                const char8* name;
                const char8* def;
            };

            Vector<Macro> macros;
            Type type;
            const char8* entry_point = "main";
			const char8* base_path = nullptr;
        };

    public:
		ShaderStage();

		~ShaderStage();

        bool load_from_bytecode(Type type, const Blob& data, const char8* name = nullptr);
		bool load_from_text(const char8* text, const CompileOptions& opts, const char8* name = nullptr);
        Blob compile_to_bytecode(const char8* text, const CompileOptions& opts);

        const Binding* find(const char8* name);
        const Binding* find_by_index(rsize idx);
		
		// Purely an utility to simplify/scode client-wise when setting parameters
		template <typename NamedCBuffer>
		ShaderVariable var()
		{
			const Binding* binding = find(NamedCBuffer::_cbuffer_name);
			if (binding) return binding->var;
			return ShaderVariable::invalid();
		}

        void destroy();

        const Binding*  get_bindings()const;
        rsize           get_bindings_count()const;

        Type type()const;

        HResource get_shader_handle()const;
        HResource get_input_signature_handle()const;

	private:
		bool _reflect_bytecode(const Blob& data, const char8* name);

        Type      m_type;
		HResource m_shader_handle;
		HResource m_input_signature_handle;
        
        Int2IntMap m_variables;
        Vector<Binding> m_bindings;
    };
}