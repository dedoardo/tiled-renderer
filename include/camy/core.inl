#include <cstring>

namespace camy
{
    namespace API
    {
		template <typename T>
		CAMY_INLINE void swap(T& a, T& b)
		{
			T c(a); a = b; b = c;
		}

        CAMY_INLINE rsize strlen(const char8* str)
        {
            return (rsize)(str == nullptr ? 0 : ::strlen(str));
        }

        CAMY_INLINE bool strcmp(const char8* a, const char8* b) { return ::strcmp(a, b) == 0; }

        template <typename T>
        CAMY_INLINE T min(const T& a, const T& b)
        {
            return a < b ? a : b;
        }

        template <typename T>
        CAMY_INLINE T max(const T& a, const T& b)
        {
            return a < b ? b : a;
        }

        template <typename T>
        CAMY_INLINE bool is_power_of_two(T val)
        {
            static_assert(std::is_integral<T>(),
                          "Power of two requires the type to be an integral");
            return (val != 0) && !(val & (val - 1));
        }
    }
}