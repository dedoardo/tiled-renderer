/* error.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/system.hpp>

#if defined(CAMY_OS_WINDOWS)
// Windows
#include <Windows.h>
#endif

namespace camy
{
    namespace API
    {
        namespace impl
        {
            std::ostream* g_log_ostream = &std::cout;
            Futex g_log_lock;

#if defined(CAMY_OS_WINDOWS)
            CONSOLE_SCREEN_BUFFER_INFO last_sbi;

            void set_info_color()
            {
                GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &last_sbi);
                HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(cout_handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }

            void set_warning_color()
            {
                GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &last_sbi);
                HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(cout_handle,
                                        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }

            void set_error_color()
            {
                GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &last_sbi);
                HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(cout_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
            }

            void restore_color()
            {
                HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(cout_handle, last_sbi.wAttributes);
            }
#endif
        }

        void log_init() { impl::g_log_lock = API::futex_create(); }

        void log_set_ostream(std::ostream* output_stream) { impl::g_log_ostream = output_stream; }

        std::ostream* log_get_ostream() { return impl::g_log_ostream; }
    }
}