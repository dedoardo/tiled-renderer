/* program.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/program.hpp>

#if defined(CAMY_OS_WINDOWS) && defined(CAMY_BACKEND_D3D11)

// D3D11 / DXGI
#define NOMINMAX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#undef NOMINMAX

namespace camy
{
    bool Program::impl_compile_stage(ShaderDesc::Type type,
                                     const CompileOpts& opts,
                                     const Blob& text,
                                     Blob& data_out)
    {
        ID3DBlob* bytecode = nullptr;
        ID3DBlob* err_msg = nullptr;
        char8* target;
        if (type == ShaderDesc::Type::Vertex)
            target = "vs_5_0";
        else if (type == ShaderDesc::Type::Pixel)
            target = "ps_5_0";
        else if (type == ShaderDesc::Type::Geometry)
            target = "gs_5_0";
        else if (type == ShaderDesc::Type::Compute)
            target = "cs_5_0";
        else
            target = "error";

        DynArray<D3D_SHADER_MACRO> macros;
        for (rsize i = 0; i < opts.num_defs; ++i)
            macros.append({opts.defs[i].name, opts.defs[i].val});
        macros.append({nullptr, nullptr});

        int flags = 0x0;
#if defined(camy_enable_layers_validation)
        flags |= (D3DCOMPILE_DEBUG);
#endif

        HRESULT result = D3DCompile(text.data, text.size, opts.base_path, macros.data(),
                                    D3D_COMPILE_STANDARD_FILE_INCLUDE, opts.entry_point, target,
                                    flags, 0, &bytecode, &err_msg);
        if (FAILED(result))
        {
            CL_ERR("D3DCompile failed with error: ", result);
            CL_ERR((char*)err_msg->GetBufferPointer());
            return false;
        }

        data_out.allocate((byte*)bytecode->GetBufferPointer(), (rsize)bytecode->GetBufferSize());
        return true;
    }

    bool Program::reflect(ShaderDesc::Type shader_type, const Blob& data, const char8* name)
    {
        rsize stype = (rsize)shader_type;
        m_variables[stype].clear();

        ID3D11ShaderReflection* shader_reflection = nullptr;
        D3DReflect(data.data, data.size, camy_uuid_ptr(shader_reflection));

        D3D11_SHADER_DESC shader_desc;
        shader_reflection->GetDesc(&shader_desc);

        UINT bound_cbuffers = shader_desc.ConstantBuffers;
        for (UINT i = 0; i < bound_cbuffers; ++i)
        {
            ID3D11ShaderReflectionConstantBuffer* cbuffer =
                shader_reflection->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC cbuffer_desc;
            cbuffer->GetDesc(&cbuffer_desc);

            if (cbuffer_desc.Type == D3D_CT_TBUFFER)
            {
                CL_WARN("TextureBuffers are not supported yet");
                continue;
            }

            if (cbuffer_desc.Type != D3D_CT_CBUFFER) continue;

            D3D11_SHADER_INPUT_BIND_DESC bind_desc;
            shader_reflection->GetResourceBindingDescByName(cbuffer_desc.Name, &bind_desc);

            Binding binding;
            binding.name = cbuffer_desc.Name;
            binding.var.type((uint32)BindType::ConstantBuffer);
            binding.var.slot(bind_desc.BindPoint);
            binding.var.size(cbuffer_desc.Size);
            binding.var.shader(shader_type);
            binding.var.uav(0);

            m_variables[stype].insert((char8*)cbuffer_desc.Name, m_bindings[stype].count());
            m_bindings[stype].append(binding);
        }

        for (UINT i = 0; i < shader_desc.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC bind_desc;
            shader_reflection->GetResourceBindingDesc(i, &bind_desc);

            uint32 is_uav = 0;
            BindType type;
            switch (bind_desc.Type)
            {
            case D3D_SIT_CBUFFER: // We evaluated cbuffers separately
            case D3D_SIT_TBUFFER: // Same for tbuffers
                continue;
            case D3D_SIT_SAMPLER:
                type = BindType::Sampler;
                break;
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                is_uav = 1;
            case D3D_SIT_STRUCTURED:
            case D3D_SIT_BYTEADDRESS:
                type = BindType::Buffer;
                break;
            case D3D_SIT_TEXTURE:
                type = BindType::Surface;
                break;
            default:
                CL_WARN("Resource: ", bind_desc.Name,
                        " has a not-recognized / not supported type, skipping");
                continue;
            }

            Binding binding;
            binding.var.type(static_cast<uint32>(type));
            binding.var.slot(bind_desc.BindPoint);
            binding.var.size(bind_desc.BindCount);
            binding.var.shader(shader_type);
            binding.var.uav(is_uav);

            // Reference by "real" name
            m_variables[stype].insert((char8*)bind_desc.Name, m_bindings[stype].count());
            m_bindings[stype].append(binding);
        }

        if (shader_type ==
            ShaderDesc::Type::Vertex) // TODO: First vertex processing stage, could be geometry
        {
            DynArray<InputElement> elements;
            for (UINT i = 0; i < shader_desc.InputParameters; ++i)
            {
                D3D11_SIGNATURE_PARAMETER_DESC param_desc;
                shader_reflection->GetInputParameterDesc(i, &param_desc);

                InputElement next;
                next.name = param_desc.SemanticName;
                next.semantic_idx = param_desc.SemanticIndex;
                next.is_instanced = false;
                next.slot = 0;

                if (param_desc.Mask == 1)
                {
                    if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                        next.type = InputElement::Type::UInt;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                        next.type = InputElement::Type::SInt;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                        next.type = InputElement::Type::Float;
                }
                else if (param_desc.Mask <= 3)
                {
                    if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                        next.type = InputElement::Type::UInt2;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                        next.type = InputElement::Type::SInt2;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                        next.type = InputElement::Type::Float2;
                }
                else if (param_desc.Mask <= 7)
                {
                    if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                        next.type = InputElement::Type::UInt3;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                        next.type = InputElement::Type::SInt3;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                        next.type = InputElement::Type::Float3;
                }
                else if (param_desc.Mask <= 15)
                {
                    if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                        next.type = InputElement::Type::UInt4;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                        next.type = InputElement::Type::SInt4;
                    else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                        next.type = InputElement::Type::Float4;
                }

                elements.append(next);
            }

            InputSignatureDesc isd;
            isd.bytecode = data;
            isd.elements = elements.data();
            isd.num_elements = elements.count();

            m_input_signature = API::rc().create_input_signature(isd, name);
            // If no elements (just hardware generated values) a invalid input layout is perfectly
            // fine
            if (isd.num_elements > 0 && m_input_signature.is_invalid())
            {
                CL_ERR("Input Signature", name);
                return false;
            }
        }

        return true;
    }
}

#endif