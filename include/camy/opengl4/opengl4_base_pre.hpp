/* opengl4_graphics_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#if defined(CAMY_BACKEND_OPENGL4)

// GLEW
#include <GL/glew.h>
#undef near
#undef far
#undef min
#undef max

namespace camy
{
	struct NativeSurface
	{
		GLuint texture;
		GLenum iformat;
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

	struct NativeBlendState { };

	struct NativeRasterizerState { };

	struct NativeDepthStencilState { };

	struct NativeInputSignature
	{
		GLuint vao;
	};

	struct NativeSampler
	{
		GLuint sampler;
	};

	struct NativeShader
	{
		GLuint shader;
	};
}

#endif