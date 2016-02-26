// Header
#include <camy/shader.hpp>

// camy
#include <camy/init.hpp>
#include <camy/gpu_backend.hpp>

// C++ STL
#include <algorithm>

namespace camy
{
	Shader::Shader() :
		// type is left uninitialized on purpose
		
		m_shader{ nullptr },
		m_input_signature{ nullptr }
	{
	
	}

	Shader::~Shader()
	{
		unload();
	}

	bool Shader::load(Type type, const void* compiled_bytecode, const Size bytecode_size)
	{
		m_type = type;

		if (compiled_bytecode == nullptr)
		{
			camy_error("Tried to load a shader with null bytecode, check your loading code!");
			return false;
		}

		unload();
		reflect(compiled_bytecode, bytecode_size);

		m_shader = hidden::gpu.create_shader(m_type, compiled_bytecode, bytecode_size);
		if (m_shader == nullptr)
		{
			unload();
			return false;
		}

		return true;
	}

	void Shader::unload()
	{
		// Shaders
		hidden::gpu.safe_dispose(m_shader);
		hidden::gpu.safe_dispose(m_input_signature);
	}

	ShaderVariable Shader::get(const char* name)const
	{
		if (m_variables.find(name) == m_variables.end())
		{
			camy_warning("Failed to retrieve ", name, " from shader, subsequent sets/binds might fail");
			return ShaderVariable(0);
		}

		// Ok why not accessing with operator [] ? the reason is that it's not const:
		// http://www.cplusplus.com/reference/unordered_map/unordered_map/operator[]/
		// Thus the compiler will refuse to compile this copnst method
		return m_variables.find(name)->second;
	}

	bool Shader::reflect(const void* compiled_bytecode, const Size bytecode_size)
	{
		// Clearing
		m_variables.clear();

		////////////////////////////////////
		
		ID3D11ShaderReflection* shader_reflection{ nullptr };
		D3DReflect(compiled_bytecode, bytecode_size, camy_uuid_ptr(shader_reflection));

		D3D11_SHADER_DESC shader_desc;
		shader_reflection->GetDesc(&shader_desc);

		auto bound_cbuffers{ shader_desc.ConstantBuffers };

		// Here we decode all the cbuffers and tbuffers, when the user wants to access them, can either access the whole buffer
		// or the single variables, this doesn't hold true
		for (auto i{ 0u }; i < bound_cbuffers; ++i)
		{
			auto cbuffer{ shader_reflection->GetConstantBufferByIndex(i) };
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

			ShaderVariable cbuffer_variable;
			// Tbuffers are bound as shaderresoures
			cbuffer_variable.type = static_cast<u32>(BindType::ConstantBuffer);
			cbuffer_variable.slot = bind_desc.BindPoint;
			cbuffer_variable.size = cbuffer_desc.Size;
			cbuffer_variable.shader_type = static_cast<u32>(m_type);
			cbuffer_variable.is_uav = 0;

			m_variables[cbuffer_desc.Name] = cbuffer_variable;
		}	

		for (auto i{ 0u }; i < shader_desc.BoundResources; ++i)
		{
			D3D11_SHADER_INPUT_BIND_DESC bind_desc;
			shader_reflection->GetResourceBindingDesc(i, &bind_desc);

			auto is_uav{ 0 };
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

			ShaderVariable shader_variable;
			shader_variable.type = static_cast<u32>(type);
			shader_variable.slot = bind_desc.BindPoint;
			shader_variable.size = bind_desc.BindCount;
			shader_variable.shader_type = static_cast<u32>(m_type);
			shader_variable.is_uav = is_uav;

			m_variables[bind_desc.Name] = shader_variable;
		}

		// It's quite easy to notice since it's camelCase
		std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc;
		for (unsigned int i = 0; i< shader_desc.InputParameters; i++)
		{
			D3D11_SIGNATURE_PARAMETER_DESC param_desc;
			shader_reflection->GetInputParameterDesc(i, &param_desc);

			D3D11_INPUT_ELEMENT_DESC element_desc;
			element_desc.SemanticName = param_desc.SemanticName;
			element_desc.SemanticIndex = param_desc.SemanticIndex;
			element_desc.InputSlot = 0;
			element_desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			element_desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			element_desc.InstanceDataStepRate = 0;

			if (param_desc.Mask == 1)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (param_desc.Mask <= 3)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (param_desc.Mask <= 7)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (param_desc.Mask <= 15)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			input_layout_desc.push_back(element_desc);
		}

		if (m_type == Shader::Type::Vertex)
		{
			camy_assert(!input_layout_desc.empty(), , "Vertex shader with no input layout?");

			// Todo : this is to allow for interleaved data, position is set to 0, all the rest is incremeneted by 1
			input_layout_desc[0].InputSlot = 0;
			for (auto i{ 1u }; i < input_layout_desc.size(); ++i)
				input_layout_desc[i].InputSlot = 1;

			m_input_signature = hidden::gpu.create_input_signature(compiled_bytecode, bytecode_size, &input_layout_desc[0], input_layout_desc.size());
			if (m_input_signature == nullptr)
			{
				camy_error("Failed to create input signature for shader");
				safe_release_com(shader_reflection);
				return false;
			}
		}

		safe_release_com(shader_reflection);
		return true;
	}
}