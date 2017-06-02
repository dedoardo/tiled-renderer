/* os.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/core.hpp>

#if CAMY_ENABLE_LOGGING > 0
#include <iostream>
#endif

namespace camy
{
    // Memory
    // -----------------------------------------------------------------------------------------------
    constexpr uint16 DEFAULT_ALIGNMENT = (uint16)-1;
    using AllocationFoundCallback = void (*)(const char8* file,
                                             uint16 line,
                                             rsize bytes,
                                             uint16 timestamp);

    namespace API
    {
        // All the following function work iff 'camy_enable_memory_tracking' is defined.
        // Starts tracking.
        CAMY_API void memory_init();

        // Returns the total number of bytes currently allocated and yet to be freed
        // if everything has been properly deallocated it returns 0.
        CAMY_API rsize memory_total_bytes();

        // Returns the number of allocated bytes for the pointer. The overhead is not included
        // this is the number passed to allocate()
        CAMY_API rsize memory_block_size(const void* ptr);

        // Iterates over the active allocations. This is useful to dump leaks at the end of an
        // application Timestamp contains the duration in milliseconds since start_tracking() was
        // called.
        CAMY_API void memory_iterate_allocations(AllocationFoundCallback callback);

        // C allocations -> no ctor/dtor called
        // C++ allocations -> ctor/dtor called w/ custom arguments
        // the first two parameters are used iff 'camy_enable_memory_tracking' is defined.
        struct _AllocationInfo
        {
            const char8* file;
            rsize count;
            uint16 alignment;
            uint16 line;
        };

        // Untyped routines
        CAMY_API void* allocate(_AllocationInfo& info);
        CAMY_API void _deallocate(void*& ptr);

        template <typename T>
        void deallocate(T*& ptr);

        template <typename T>
        inline void deallocate(T*& ptr);

        template <typename T, typename... Ts>
        inline T* tallocate(_AllocationInfo& info, Ts&&... args);

        template <typename T>
        inline void tdeallocate(T*& ptr);

        template <typename T, typename... Ts>
        inline T* tallocate_array(_AllocationInfo& info, Ts&&... args);

        template <typename T>
        inline void tdeallocate_array(T*& ptr);
/*
 * How to use them:
 * allocate expects count to be the number of bytes
 * tallocate expects count to be 1 -> allocate(sizeof(T))
 * tallocate_array expected count to be the number of elements -> allocate(count * sizeof(T))
 */
#define CAMY_ALLOC(count, alignment)                                                               \
    ::camy::API::_AllocationInfo { __FILE__, (count), (alignment), (uint16)__LINE__ }
#define CAMY_UALLOC(count) CAMY_ALLOC(count, ::camy::DEFAULT_ALIGNMENT)
#define CAMY_ALLOC1(alignment)                                                                     \
    ::camy::API::_AllocationInfo { __FILE__, 1, (alignment), (uint16)__LINE__ }
#define CAMY_UALLOC1 CAMY_ALLOC1(::camy::DEFAULT_ALIGNMENT)
    }

    // File
    // -----------------------------------------------------------------------------------------------
    CAMY_ENUM_BITS(FileOpen, uint32){// Reading
                                     Read = 1,

                                     // Writing
                                     Write = 1 << 1,

                                     // Fails if file does not exist
                                     Exists = 1 << 2};

    // Offset bytes are offset from when reading/writing
    enum class FileSeekOff
    {
        Start,
        Cur,
        End
    };

    // Timestamp
    struct FileTimestamp
    {
        uint16 year;
        uint8 month;
        uint8 day;
        uint8 hour;
        uint8 minute;
        uint8 seconds;
        uint8 _padding0;
    };

    // File info
    struct FileInfo
    {
        rsize size;
        FileTimestamp created;
        FileTimestamp last_modified;
        bool is_directory;
    };

#pragma pack(push, 1)
    struct HFile
    {
        // Opaque, probably a system handle, do not use this
        uint64 _v;

        // Creates a new invalid file handle
        HFile(uint64);

        // True if valid
        bool is_valid() const;

        // True if invalid
        bool is_invalid() const;
    };
#pragma pack(pop)

    namespace API
    {
        // Checks whether the specified file exists and is supported
        CAMY_API bool file_exists(const char8* path);

        // Deletes the file. No warning is issued if the file does not exist.
        CAMY_API void file_delete(const char8* path);

        // Retrieves useful information about the file w/o opening it
        CAMY_API bool file_get_info(const char8* path, FileInfo& info);

        // Opens a file for subsequent operations as specified by opts
        CAMY_API HFile file_open(const char8* path, int opts);

        // Closes a previously opened file handle. No warning issued if handle is invalid
        CAMY_API void file_close(HFile hfile);

        // Writes bytes_to_write from buffer into a valid HFile, returns the number of bytes
        // writtern
        CAMY_API rsize file_write(HFile hfile, const byte* buffer, rsize bytes_to_write);

        // Reads bytes_to_read into buffer from a valid HFile, returns the number of bytes read
        CAMY_API rsize file_read(HFile hfile, byte* buffer, rsize bytes_to_read);

        /*
         * Path utility that returns the filename given the path.
         * the pointer returned is part of the same input buffer
         * This is a very trivial implementation, extracting a path is very straightforward
         * 99% of the cases and that's exactly what's covered.
         * Both forward and backward slash separators are supported
         * Device specification as in Windows is supported aswell
         * UNC works out of the box as it is treated as a relative path
         * Note: Assuming the string is null-terminated
         * Example: C:\\Users\\test\\Documents\\file.obj.ext returns file.obj.ext
         * As the buffer returns is simply an incremented pointer
         */
        CAMY_API const char8* path_extract_filename(const char8* path);

        CAMY_API const char8* path_extract_extension(const char8* path);
    }

    // Thread
    // -----------------------------------------------------------------------------------------------
    using ThreadID = uint64;
    using ThreadProc = bool (*)(void*);

    using Futex = void*;

    template <typename T>
    using Atomic = volatile T;

    namespace API
    {
        CAMY_API ThreadID thread_launch(ThreadProc proc, void* pdata);
        CAMY_API void thread_join(ThreadID thread);
        CAMY_API bool thread_is_valid(ThreadID thread);
        CAMY_API ThreadID thread_current();
        CAMY_API void thread_yield_current();
        CAMY_API void thread_sleep_current(rsize milliseconds);

        CAMY_API void debug_break();

        /*
         * On x86 the instruction sequences load->load, load->store, store->store are
         * all guaranteed to not be reordered by the hardware, so only the compiler has
         * to be notified for these.
         */
        CAMY_API void compiler_fence();

        // On x86 a store->load can be reordered by hardware, it needs its own memory fence
        CAMY_API void memory_fence();

        CAMY_API uint32 atomic_cas(uint32& data, uint32 expected, uint32 desired);
        CAMY_API uint32 atomic_swap(uint32& data, uint32 desired);
        CAMY_API uint32 atomic_fetch_add(uint32& data, uint32 addend);

        CAMY_API Futex futex_create();
        CAMY_API void futex_destroy(Futex futex);
        CAMY_API void futex_lock(Futex futex);
        CAMY_API void futex_unlock(Futex futex);

#ifdef CAMY_ENABLE_ASSERTS
#define CAMY_ASSERT(x)                                                                             \
    {                                                                                              \
        if (!(x)) (::camy::API::debug_break());                                                    \
    }
#else
#define camy_assert(x) camy_error("FAILED ASSERTION")
#endif
    }

    /* Time
    * Simple high-resolution clock with minimal interface
    * Usage:
    * TimeSlice a = timer_split();
    * TimeSlice b = timer_split();
    * float milliseconds_delta = timer_elapsed(b - a);

    * Note: It's not recommended to rely on TimeSlice for anything
    * else that calculating elapsed time . calling timer_elapsed(timer_split())
    * is not guaranteed to return anythign meaningful.
    * ---------------------------------------------------------------------------------------------
    */
    using TimeSlice = sint64;

    namespace API
    {
        // Returns an integer identifying the current timepoint (in ticks usually)
        TimeSlice timer_split();

        // Returns the time in millisecond elapsed in a specific delta.
        // the slice passed should always be a different between different time points
        float timer_elapsed(TimeSlice slice);
    }

    // Log
    // -----------------------------------------------------------------------------------------------
    namespace API
    {
        CAMY_API void log_init();
        CAMY_API void log_set_ostream(std::ostream* output_stream);
        CAMY_API std::ostream* log_get_ostream();
        template <typename... Ts>
        void log(const char8* tag, const char8* fun, int line, Ts&&... args);

#if CAMY_ENABLE_LOGGING >= 1
#define CL_ERR(...) ::camy::API::log("Err ", __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define CL_ERR(...)
#endif

#if CAMY_ENABLE_LOGGING >= 2
#define CL_WARN(...) ::camy::API::log("Warn", __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define CL_WARN(...)
#endif

#if CAMY_ENABLE_LOGGING >= 3
#define CL_INFO(...) ::camy::API::log("Info", __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define CL_INFO(...)
#endif
    }
}

#include "system.inl"