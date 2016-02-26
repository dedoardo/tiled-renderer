#pragma once

// camy
#include "base.hpp"
#include "resources.hpp"
#include "features.hpp"

// C++ STL
#include <unordered_map>

namespace camy
{
	// Forward declaration
	class GPUBackend;

	struct ShaderVariable
	{
		// Valid by default
		ShaderVariable(u32 valid = 1) : valid { valid } { }

		u32 type : 2;
		static_assert(2 << 2 >= static_cast<u32>(BindType::LastValue), "ShaderVariable::type too small to contains all possible types");

		u32 slot : 7;
		static_assert(2 << 7 >= features::max_bindable_shader_resources, "ShaderVariable::index has not enough bit reserved");

		u32 size : 16;
		static_assert(2 << 16 >= features::max_cbuffer_size, "ShaderVariable::offset too small to contain possible variable offset from parent structure");
	
		u32 valid : 1;

		u32 shader_type : 3;

		u32 is_uav : 1;
		u32 padding : 2;
	};
	static_assert(sizeof(ShaderVariable) == 4, "ShaderVariable is bigger than expected, if you think this is not an issue feel free to remove this very assert");

	/*
		Class: Shader
			Abstraction over raw shader handles and input signature that allows for easier
			cbuffer update and name => shader resource / variable mapping. It allows us to have
			different levels of "security" based on the mode we are running on: e.g. on Test mode, 
			each time a parameter is set its details are checked against the one obtained during reflection
			is release mode we simply trust the user. The Shader itself is a blackbox mostly because it is 
			one of the very few pieces that directly interacts with the D3D11 and it's simply a useful addition
	*/
	class Shader final
	{
		friend class GPUBackend;
	
	public:
		/*
			Enumerator: Type
				Type of the shader
		*/
		enum class Type : u32
		{
			Vertex = 0,
			Geometry = 1,
			Pixel = 2,
			Compute
		};

		static const u32 num_types{ 4 };
	
	public:
		/*
			Constructor: Shader
				Constructs a new Shader instance of the specified type, nothing is initialized,
				//? Todo : Think whether to move type in the load() 
		*/
		Shader();

		/*
			Destrutor: ~Shader
				Releases all the internal resources associated with the shader, calls dispose()
		*/
		~Shader();

		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;

		Shader(Shader&& other) = default;
		Shader& operator=(Shader&& other) = default;

		/*
			Function: load
				Loads the specified shader creating initializing all the internal structures. First a dispose() is called
		*/
		bool load(Type type, const void* bytecode, const Size bytecode_size);

		/*
			Function: dispose
				Releases all the resources used with the shader, there is no need to call this manually
				everything is done by the constructor. 
		*/
		void unload();

		/*
			Function: get
				Retrieves any resource that the shader is expecting and will possibly use, note that
				shader compilers usually remove unused variable

			variable_name - ASCII name of the variable
		*/
		ShaderVariable get(const char* variable_name)const;

		Type get_type()const { return m_type; }

	private:
		bool reflect(const void* compiled_bytecode, const Size bytecode_size);

	private:
		/*
			Var: m_type
				Type of the shader see Shader::Type
		*/
		Type m_type;

		/*
			Var: m_shader
				Internal handle for the shader ( m_gpu_backend.get_resource_allocator() ) 
		*/
		hidden::Shader* m_shader;

		/*
			Var: m_input_signature
				Input signature, valid iff the shader is of type Vertex
		*/
		InputSignature*	m_input_signature;
		
		/*
			Var: m_variables
				Mapping from variable name to ShaderVariable that can be used for binding / setting
		*/
		std::unordered_map<std::string, ShaderVariable> m_variables;
	};
}

#include "shader.inl"