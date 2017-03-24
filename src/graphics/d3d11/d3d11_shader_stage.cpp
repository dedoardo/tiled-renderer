/* shader_stage.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/shader_stage.hpp>

#if defined(camy_os_windows) && defined(camy_backend_d3d11)

// camy
#include <camy/graphics/platform.hpp>
#include <camy/graphics/render_context.hpp>
#include <camy/core/memory/linear_vector.hpp>
#include <camy/core/memory/auto_ptr.hpp>
#include <camy/graphics/resource_manager.hpp>

// D3D11 / DXGI
#define NOMINMAX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#undef NOMINMAX

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

		m_type = type;

        // Reflecting
        _reflect_bytecode(data, name);

        // Creating shader
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
        ID3DBlob* bytecode = nullptr;
        ID3DBlob* err_msg = nullptr;
		char8* target;
        if (opts.type == Type::Vertex) target = "vs_5_0";
        else if (opts.type == Type::Pixel) target = "ps_5_0";
        else if (opts.type == Type::Geometry) target = "gs_5_0";
        else if (opts.type == Type::Compute) target = "cs_5_0";
        else target = "error";

        ArrayAutoPtr<D3D_SHADER_MACRO> macros;
        HRESULT res = D3DCompile(text, ::camy::strlen(text), opts.base_path,
            macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, opts.entry_point, target, 0, 0, &bytecode, &err_msg);
        if (FAILED(res))
        {
            camy_error("Failed to compile shader:");
            camy_error_stripped((char8*)err_msg->GetBufferPointer());
            return Blob();
        }

        Blob ret;
        ret.allocate_data((byte*)bytecode->GetBufferPointer(), (rsize)bytecode->GetBufferSize());
        return ret;
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
        API::rc().destroy_input_signature(m_input_signature_handle);
        API::rc().destroy_shader(m_shader_handle);
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

        ////////////////////////////////////
        ID3D11ShaderReflection* shader_reflection{ nullptr };
        D3DReflect(data.data, data.byte_size, camy_uuid_ptr(shader_reflection));

        D3D11_SHADER_DESC shader_desc;
        shader_reflection->GetDesc(&shader_desc);

        UINT bound_cbuffers = shader_desc.ConstantBuffers;

        // Here we decode all the cbuffers and tbuffers, when the user wants to access them, can either access the whole buffer
        // or the single variables, this doesn't hold true
        for (UINT i = 0; i < bound_cbuffers; ++i)
        {
            ID3D11ShaderReflectionConstantBuffer* cbuffer = shader_reflection->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC cbuffer_desc;
            cbuffer->GetDesc(&cbuffer_desc);

            switch (cbuffer_desc.Type)
            {
            case D3D_CT_TBUFFER:
                camy_warning("TextureBuffers are not supported yet");
                continue;
            case D3D_CT_CBUFFER:
                break;
            default:
                // This warning pollutes the output when reflecting compute shaders, because any RWStructuredBuffer ( maybe others aswell, haven't tested ) 
                // will be passed as Constant buffer, thus i'm not outputing any warning
                continue;
            }

            // To find the binding slot i have to query the resource type, index does not refer to binding point
            D3D11_SHADER_INPUT_BIND_DESC bind_desc;
            shader_reflection->GetResourceBindingDescByName(cbuffer_desc.Name, &bind_desc);

            // Tbuffers are bound as shaderresoures
            Binding binding;
            binding.var.type((uint32)BindType::ConstantBuffer);
            binding.var.slot(bind_desc.BindPoint);
            binding.var.size(cbuffer_desc.Size);
            binding.var.shader(m_type);
            binding.var.uav(0);

            for (UINT cb_var = 0; cb_var < cbuffer_desc.Variables; ++cb_var)
            {
                ID3D11ShaderReflectionVariable* var = cbuffer->GetVariableByIndex(cb_var);

                D3D11_SHADER_VARIABLE_DESC var_desc;
                var->GetDesc(&var_desc);

                Binding constant_binding;
                constant_binding.var.type((uint32)BindType::Constant);
                constant_binding.var.slot(bind_desc.BindPoint);
                constant_binding.var.size(cbuffer_desc.Size);
                constant_binding.var.shader(m_type);
                constant_binding.var.uav(0);

                constant_binding.constant.name = var_desc.Name;
                constant_binding.constant.offset = var_desc.StartOffset;
                constant_binding.constant.size = var_desc.Size;

                binding.constants.append(var_desc.Name);
                m_variables.insert((char8*)var_desc.Name, m_bindings.count());
                m_bindings.append(constant_binding);
            }

            // reference by "real" name
            m_variables.insert((char8*)cbuffer_desc.Name, m_bindings.count());

            // reference by bind name
            VarName name = _camy_default_binding;
            name.append(bind_desc.BindPoint);
            m_variables.insert((char8*)name.str(), m_bindings.count());
            m_bindings.append(binding);
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
                type = BindType::Sampler; break;
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                is_uav = 1;
            case D3D_SIT_STRUCTURED:
            case D3D_SIT_BYTEADDRESS:
                type = BindType::Buffer; break;
            case D3D_SIT_TEXTURE:
                type = BindType::Surface; break;
            default:
                camy_warning("Resource: ", bind_desc.Name, " has a not-recognized / not supported type, skipping"); continue;
            }

            Binding binding;
            binding.var.type(static_cast<uint32>(type));
            binding.var.slot(bind_desc.BindPoint);
            binding.var.size(bind_desc.BindCount);
            binding.var.shader(m_type);
            binding.var.uav(is_uav);

            // Reference by "real" name
            m_variables.insert((char8*)bind_desc.Name, m_bindings.count());

            // Reference by binding name
            VarName name = _camy_default_binding;
            name.append(bind_desc.BindPoint);
            m_variables.insert((char8*)name.str(), m_bindings.count());

            m_bindings.append(binding);
        }

        LinearVector<InputElement> elements(shader_desc.InputParameters);
        for (UINT i = 0; i< shader_desc.InputParameters; i++)
        {
            D3D11_SIGNATURE_PARAMETER_DESC param_desc;
            shader_reflection->GetInputParameterDesc(i, &param_desc);

            InputElement& next = elements.next();
            next.name = param_desc.SemanticName;
            next.semantic_idx = param_desc.SemanticIndex;
            next.is_instanced = false; // TODO: Implement shader metadata
            next.slot = 0; // TODO: Implement shader metadata

            if (param_desc.Mask == 1)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) next.type = InputElement::Type::UInt;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) next.type = InputElement::Type::SInt;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) next.type = InputElement::Type::Float;
            }
            else if (param_desc.Mask <= 3)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) next.type = InputElement::Type::UInt2;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) next.type = InputElement::Type::SInt2;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) next.type = InputElement::Type::Float2;
            }
            else if (param_desc.Mask <= 7)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) next.type = InputElement::Type::UInt3;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) next.type = InputElement::Type::SInt3;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) next.type = InputElement::Type::Float3;
            }
            else if (param_desc.Mask <= 15)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) next.type = InputElement::Type::UInt4;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) next.type = InputElement::Type::SInt4;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) next.type = InputElement::Type::Float4;
            }
        }

        if (m_type == ShaderDesc::Type::Vertex)
        {
            camy_assert(elements.count() > 0);

            StaticString<60> is_name = name;
            is_name.append("_is_$");

            InputSignatureDesc isd;
            isd.bytecode = data;
            isd.elements = elements.data();
            isd.num_elements = elements.count();

            m_input_signature_handle = API::rc().create_input_signature(isd, is_name.str());
            if (m_input_signature_handle == kInvalidHResource)
            {
                camy_error("Failed to create InputSignature: ", is_name);
                goto error;
            }
        }

        if (shader_reflection != nullptr)
            shader_reflection->Release();
        return true;

    error:
        if (shader_reflection != nullptr)
            shader_reflection->Release();
        API::rc().destroy_input_signature(m_input_signature_handle);
        API::rc().destroy_shader(m_shader_handle);
        return false;
    }
}

#endif