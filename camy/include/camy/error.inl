#include "error.hpp"
namespace camy
{
	namespace hidden
	{
		template<typename OutArg>
		void log_rec(const OutArg & arg)
		{
			*g_output_stream << arg;
		}

		template <typename FirstOutArg, typename SecondOutArg, typename ...OutArgs>
		void log_rec(const FirstOutArg& first_out_arg, const SecondOutArg& second_out_arg, const OutArgs& ...out_args)
		{
			*g_output_stream << first_out_arg;
			log_rec(second_out_arg, out_args...);
		}

		template <typename ...OutArgs>
		void log(const char8* tag, const char8* function, const char8* line, const OutArgs& ...out_args)
		{
			auto tag_len{ std::strlen(tag) };
			auto max_len{ strlen("Warning") };
			std::string empty(max_len - tag_len, ' ');

			if (g_output_stream == nullptr) return;
			*g_output_stream << "[" << tag << "] " << empty;

			log_rec(out_args...);

			*g_output_stream << " [" << function << ":" << line << "]";
			*g_output_stream << std::endl;
		}
	}
}