/* file.cpp
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

namespace camy
{
    HFile::HFile(uint64 val) : _v(val) {}

    bool HFile::is_valid() const { return _v != (uint64)INVALID_HANDLE_VALUE; }

    bool HFile::is_invalid() const { return _v == (uint64)INVALID_HANDLE_VALUE; }

    namespace API
    {
        bool file_exists(const char8* path)
        {
            if (path == nullptr)
            {
                CL_ERR("Invalid argument: path is null");
                return false;
            }

            HANDLE fhandle =
                CreateFile(path, 0x0, 0x0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

            if (fhandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(fhandle);
                return true;
            }

            DWORD err = GetLastError();
            if (err == ERROR_FILE_NOT_FOUND) return false;

            CL_ERR("Failed to determine if file: ", path, " exists, error: ", err);
            return false;
        }

        void file_delete(const char8* path)
        {
            if (path == nullptr)
            {
                CL_ERR("Invalid argument: path is null");
                return;
            }

            DeleteFileA(path);
        }

        bool file_get_info(const char8* path, FileInfo& info)
        {
            if (path == nullptr)
            {
                CL_ERR("Invalid argument: path is null");
                return false;
            }

            WIN32_FILE_ATTRIBUTE_DATA fdata;

            if (!GetFileAttributesEx(path, GetFileExInfoStandard, &fdata))
            {
                // camy_err("Failed to retrieve data for file: ", filename, " error: ",
                // GetLastError());
                return false;
            }

            info.is_directory = fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            ((ULARGE_INTEGER&)info.size).LowPart = fdata.nFileSizeLow;
            ((ULARGE_INTEGER&)info.size).HighPart = fdata.nFileSizeHigh;

            SYSTEMTIME creation_time;
            FileTimeToSystemTime(&fdata.ftCreationTime, &creation_time);

            SYSTEMTIME last_time;
            FileTimeToSystemTime(&fdata.ftLastAccessTime, &last_time);

            info.created.year = (uint16)creation_time.wYear;
            info.created.month = (uint8)creation_time.wMonth;
            info.created.day = (uint8)creation_time.wHour;
            info.created.hour = (uint8)creation_time.wHour;
            info.created.minute = (uint8)creation_time.wMinute;
            info.created.seconds = (uint8)creation_time.wSecond;

            info.last_modified.year = (uint16)last_time.wYear;
            info.last_modified.month = (uint8)last_time.wMonth;
            info.last_modified.day = (uint8)last_time.wHour;
            info.last_modified.hour = (uint8)last_time.wHour;
            info.last_modified.minute = (uint8)last_time.wMinute;
            info.last_modified.seconds = (uint8)last_time.wSecond;

            return true;
        }

        HFile file_open(const char8* path, int opts)
        {
            DWORD access = 0x0;
            if (opts & FileOpen::Read) access |= GENERIC_READ;
            if (opts & FileOpen::Write) access |= GENERIC_WRITE;

            DWORD dispositions = OPEN_ALWAYS;
            if (opts & FileOpen::Exists) dispositions = OPEN_EXISTING;

            HANDLE fhandle =
                CreateFile(path, access, 0, nullptr, dispositions, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fhandle == INVALID_HANDLE_VALUE)
            {
                CL_ERR("Win32::CreateFile() failed with code: ", GetLastError());
                return HFile((uint64)INVALID_HANDLE_VALUE);
            }

            return HFile((uint64)fhandle);
        }

        void file_close(HFile hfile)
        {
            if (hfile.is_invalid()) return;

            CloseHandle((HANDLE)hfile._v);
        }

        rsize file_write(HFile hfile, const byte* buffer, rsize bytes_to_write)
        {
            if (hfile.is_invalid() || buffer == nullptr || bytes_to_write == 0) return 0;

            DWORD written = 0;
            if (!WriteFile((HANDLE)hfile._v, buffer, (DWORD)bytes_to_write, &written, nullptr))
                CL_ERR("Win32::WriteFile failed with error: ", GetLastError());

            return (int)written;
        }

        rsize file_read(HFile hfile, byte* buffer, rsize bytes_to_read)
        {
            if (hfile.is_invalid() || buffer == nullptr || bytes_to_read == 0) return 0;

            DWORD read = 0;
            if (!ReadFile((HANDLE)hfile._v, buffer, bytes_to_read, &read, nullptr))
                CL_ERR("Win32::ReadFile failed with error: ", GetLastError());

            return (int)read;
        }
    }
}
#else
#error No implementation available for this platform
#endif

namespace camy
{
    namespace API
    {
        const char8* path_extract_filename(const char8* path)
        {
            if (path == nullptr || *path == '\0')
            {
                CL_ERR("Invalid argument: path is null or empty");
                return nullptr;
            }

            const char8* cur = path;

            // Checking if there is a device name
            // [A-Z]: Ignoring : as ':' is a reserved character
            // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#file_and_directory_names
            if (*(path + 1) == ':') cur += 2;

            const char8* ret = cur;
            // Navigating directories
            while (true)
            {
                char ch = *cur;
                // EOS
                if (ch == '\0') break;

                // Everytime we find a new separator we set the ret to
                // the character right after, eventually it will return the filename w/ extension.
                if (ch == '\\' || ch == '/') ret = cur + 1;

                ++cur;
            }

            return ret;
        }

        const char8* path_extract_extension(const char8* path)
        {
            const char8* cur = path_extract_filename(path);

            const char8* ret = cur;
            while (true)
            {
                char ch = *cur;

                if (ch == '\0') break;

                if (ch == '.') ret = cur + 1;

                ++cur;
            }

            return ret;
        }
    }
}