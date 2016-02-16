namespace camy
{
	template <typename ValueType>
	void Shader::set(ShaderVariable variable, const ValueType& data)
	{
		if (std::is_pointer(data))
			camy_warning("Tried to set a variable as a pointer, you probably didn't intend to use this function, if you really intend to, feel free to remove this warning")
		else
			set(variable, &data, sizeof(ValueType));
	}
}