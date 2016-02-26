// Header
#include <camy_render/shader_common.hpp>

namespace camy
{
	namespace shaders
	{
		const char* PerFrame::name{ "PerFrame" };
		const char* PerFrameView::name{ "PerFrameView" };
		const char* PerFrameLight::name{ "PerFrameLight" };
		const char* PerObject::name{ "PerObject" };
		const char* PerFrameAndObject::name{ "PerFrameAndObject" };
		const char* Material::name{ "Material" };
		const char* Environment::name{ "Environment" };
		const char* CullingDispatchArgs::name{ "CullingDispatchArgs" };
		const char* KawaseBlurArgs::name{ "KawaseBlurArgs" };
	}
}