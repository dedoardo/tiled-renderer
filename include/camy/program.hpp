/* program.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/containers/blob.hpp>
#include <camy/containers/hash_map.hpp>
#include <camy/containers/static_string.hpp>
#include <camy/containers/dyn_array.hpp>
#include <camy/graphics.hpp>
#include <camy/render_context.hpp>

#define CAMY_CBUFFER(type) #type

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
        bool compile_stage(ShaderDesc::Type type,
                           const CompileOpts& opts,
                           const Blob& desc,
                           Blob& data_out);
        void unload();

        ShaderVariable var(ShaderDesc::Type type, const char8* name);
        HResource input_signature();

        HResource shader(ShaderDesc::Type type);

        // Helpers
        HResource vs();
        HResource ps();
        ShaderVariable vertex_var(const char8* name);
        ShaderVariable pixel_var(const char8* name);

      private:
        //! Platform dependent implementation
        bool impl_compile_stage(ShaderDesc::Type type,
                                const CompileOpts&,
                                const Blob& desc,
                                Blob& data_out);
        bool reflect(ShaderDesc::Type type, const Blob& data, const char8* name);

        HResource m_input_signature;
        HResource m_shaders[(rsize)ShaderDesc::Type::Count];

        HashMap<uint64> m_variables[(rsize)ShaderDesc::Type::Count];
        DynArray<Binding> m_bindings[(rsize)ShaderDesc::Type::Count];
    };
}