/* program.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/program.hpp>

#if defined(camy_os_windows) && defined(camy_backend_opengl4)

// camy
#include <camy/graphics/platform.hpp>
#include <camy/graphics/render_context.hpp>

namespace camy
{
	bool Program::impl_compile_stage(ShaderDesc::Type type, const CompileOpts& opts, const Blob& text, Blob& data_out)
	{
		return true;
	}

	bool Program::reflect(ShaderDesc::Type shader_type, const Blob& data, const char8* name)
	{
		return true;
	}
}
#endif