// Header
#include <camy/error.hpp>

// Windows
#include <Windows.h>

namespace camy
{
	namespace hidden
	{
		std::ostream* g_output_stream{ &std::cout };

		void set_output_stream(std::ostream* output_stream)
		{
			g_output_stream = output_stream;
		}

		std::ostream* get_output_stream()
		{
			return g_output_stream;
		}

		void log(const char8* tag, const char8* function, const char8* line, std::initializer_list<const char8*> messages)
		{
			auto tag_len{ std::strlen(tag) };
			auto max_len{ std::strlen("Warning") };
			std::string empty(max_len - tag_len, ' ');

			if (g_output_stream == nullptr) return;
			*g_output_stream << "[" << tag << "] " << empty;
			for (const auto& message : messages)
				*g_output_stream << message;
			*g_output_stream << " [" << function << ":" << line << "]";
			*g_output_stream << std::endl;
		}

		void debug_break()
		{
			DebugBreak();
		}
	}
}