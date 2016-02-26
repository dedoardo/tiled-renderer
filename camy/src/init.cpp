// Header
#include <camy/init.hpp>

namespace camy
{
	namespace hidden
	{
		GPUBackend gpu;
	}

	bool init(u32 adapter_index)
	{
		return hidden::gpu.open(adapter_index);
	}

	void shutdown()
	{
		hidden::gpu.close();
	}
}