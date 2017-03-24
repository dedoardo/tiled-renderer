/* opengl4_graphics_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// OpenGL
#include <GL/glew.h>
#undef near
#undef far

namespace camy
{
	constexpr uint32 kMaxConcurrentContexts = 2u;

	struct NativeSurface
	{
		GLuint frame_buffer;
		GLuint texture_buffer;
		GLuint depth_buffer;
	};

	struct NativeBuffer
	{
		GLuint buffer;
	};

	struct NativeVertexBuffer
	{
		GLuint buffer;
	};

	struct NativeIndexBuffer
	{
		GLuint buffer;
	};

	struct NativeInstanceBuffer
	{
		GLuint buffer;
	};

	struct NativeConstantBuffer
	{
		GLuint buffer;
	};

	struct NativeBlendState
	{
		
	};

	struct NativeRasterizerState
	{

	};

	struct NativeDepthStencilState
	{

	};

	struct NativeInputSignature
	{

	};

	struct NativeSampler
	{

	};

	struct NativeShader
	{
		GLuint shader;
	};
}