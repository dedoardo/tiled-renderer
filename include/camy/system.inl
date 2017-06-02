namespace camy
{
    namespace API
    {
        // Memory
        template <typename T>
        CAMY_INLINE void deallocate(T*& ptr)
        {
            if (ptr == nullptr) return;
            _deallocate((void*&)ptr);
            ptr = nullptr;
        }

        template <typename T, typename... Ts>
        CAMY_INLINE T* tallocate(_AllocationInfo& info, Ts&&... args)
        {
            if (info.alignment == DEFAULT_ALIGNMENT) info.alignment = alignof(T);

            CAMY_ASSERT(info.count == 1);
            info.count = sizeof(T);
            void* ptr = allocate(info);
            new (ptr) T(std::forward<Ts>(args)...);
            return (T*)ptr;
        }

        template <typename T>
        CAMY_INLINE void tdeallocate(T*& ptr)
        {
            if (ptr == nullptr) return;
            ptr->~T();
            deallocate(ptr);
            ptr = nullptr;
        }

        template <typename T, typename... Ts>
        CAMY_INLINE T* tallocate_array(_AllocationInfo& info, Ts&&... args)
        {
            if (info.alignment == DEFAULT_ALIGNMENT) info.alignment = alignof(T);

            rsize num_elements = info.count;
            info.count *= sizeof(T);
            void* ptr = allocate(info);
            for (rsize i = 0; i < num_elements; ++i)
                new ((T*)ptr + i) T(std::forward<Ts>(args)...);
            return (T*)ptr;
        }

        template <typename T>
        CAMY_INLINE void tdeallocate_array(T*& ptr)
        {
            if (ptr == nullptr) return;

            rsize bytes = memory_block_size(ptr);
            CAMY_ASSERT(bytes % sizeof(T) == 0);
            rsize count = bytes / sizeof(T);
            for (rsize i = 0; i < count; ++i)
                ((T*)ptr + i)->~T();
            deallocate(ptr);
            ptr = nullptr;
        }

        // Log
        namespace impl
        {
			extern std::ostream* g_log_ostream;
			extern Futex g_log_lock;

            void set_info_color();
            void set_warning_color();
            void set_error_color();
            void restore_color();

            template <typename OutArg>
            void log_rec(const OutArg* arg)
            {
                if (arg == nullptr)
                    *g_log_ostream << "<none>";
                else
                    *g_log_ostream << arg;
            }

            template <typename OutArg>
            void log_rec(const OutArg& arg)
            {
                *g_log_ostream << arg;
            }

            template <typename FirstOutArg, typename SecondOutArg, typename... OutArgs>
            void log_rec(const FirstOutArg* first_out_arg,
                         const SecondOutArg& second_out_arg,
                         const OutArgs&... out_args)
            {
                if (first_out_arg == nullptr)
                    *g_log_ostream << "<none>";
                else
                    *g_log_ostream << first_out_arg;
                log_rec(second_out_arg, out_args...);
            }

            template <typename FirstOutArg, typename SecondOutArg, typename... OutArgs>
            void log_rec(const FirstOutArg& first_out_arg,
                         const SecondOutArg& second_out_arg,
                         const OutArgs&... out_args)
            {
                *g_log_ostream << first_out_arg;
                log_rec(second_out_arg, out_args...);
            }

            CAMY_INLINE void log_header(const char8* tag, const char8* fun, int line)
            {
                std::ostream& os = *g_log_ostream;

                if (g_log_ostream == &std::cout)
                {
                    if (API::strcmp(tag, "Info"))
                        set_info_color();
                    else if (API::strcmp(tag, "Warn"))
                        set_warning_color();
                    else if (API::strcmp(tag, "Err "))
                        set_error_color();
                }

                os << "[" << tag << "]";
            }

            CAMY_INLINE void log_footer(const char8* tag, const char8* fun, int line)
            {
                std::ostream& os = *g_log_ostream;

                // [namespace::to::func@line]
                os << " [" << fun << "@" << line << "]";

                // Newline
                os << std::endl;

                // Previous color saved in header
                if (g_log_ostream == &std::cout) restore_color();
            }
		}

		template <typename... Ts>
		CAMY_INLINE void log(const char8* tag, const char8* fun, int line, Ts&&... args)
		{
			if (impl::g_log_ostream == nullptr) return;

			//------------------------------
			API::futex_lock(impl::g_log_lock);
			impl::log_header(tag, fun, line);
			impl::log_rec(args...);
			impl::log_footer(tag, fun, line);
			API::futex_unlock(impl::g_log_lock);
			//------------------------------
		}
    }
}