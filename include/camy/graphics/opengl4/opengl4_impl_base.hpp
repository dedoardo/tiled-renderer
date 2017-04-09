/* opengl4_impl_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#if defined(camy_os_windows) && defined(camy_backend_opengl4)

// OGL
#include <GL/glew.h>
#include <GL/wglew.h>
#undef near
#undef far
#undef min
#undef max

namespace camy
{
	struct camy_api	RenderContextData
	{
		HGLRC hrc;
		Surface      surface;
	};

	struct camy_api CommandListData
	{

	};

	inline bool gl_err()
	{
		GLenum error = glGetError();

		if (error == GL_NO_ERROR)
			return false;

		while (error != GL_NO_ERROR)
		{
			char* error_str;
			switch (error)
			{
				case GL_INVALID_OPERATION:      error_str = "INVALID_OPERATION";      break;
				case GL_INVALID_ENUM:           error_str = "INVALID_ENUM";           break;
				case GL_INVALID_VALUE:          error_str = "INVALID_VALUE";          break;
				case GL_OUT_OF_MEMORY:          error_str = "OUT_OF_MEMORY";          break;
				case GL_INVALID_FRAMEBUFFER_OPERATION:  error_str = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cl_system_err("OpenGL4.5", error_str, "");
			error = glGetError();
		}

		return true;
	}
}

#endif