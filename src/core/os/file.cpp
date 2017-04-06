/* file.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/os/file.hpp>

#if defined(camy_os_windows)
#include <Windows.h>

namespace camy
{
	HFile::HFile(uint64 val) : _v(val) { } 

	bool HFile::is_valid()const
	{
		return _v != (uint64)INVALID_HANDLE_VALUE;
	}

	bool HFile::is_invalid()const
	{
		return _v == (uint64)INVALID_HANDLE_VALUE;
	}

	bool file_exists(const char8* path)
	{
		if (path == nullptr)
		{
			cl_invalid_arg(path);
			return false;
		}

		HANDLE fhandle = CreateFile(path, 0x0, 0x0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		if (fhandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(fhandle);
			return true;
		}

		DWORD err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND)
			return false;

		cl_internal_err("Failed to properly determine if file: ", path, " exists, error: ", err);
		return false;
	}

	void file_delete(const char8* path)
	{
		if (path == nullptr)
		{
			cl_invalid_arg(path);
			return;
		}

		DeleteFileA(path);
	}

	bool file_get_info(const char8* path, FileInfo& info)
	{
		if (path == nullptr)
		{
			cl_invalid_arg(path);
			return false;
		}

		WIN32_FILE_ATTRIBUTE_DATA fdata;

		if (!GetFileAttributesEx(path, GetFileExInfoStandard, &fdata))
		{
			//camy_err("Failed to retrieve data for file: ", filename, " error: ", GetLastError());
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
		if (opts & FileOpen_Read)
			access |= GENERIC_READ;
		if (opts & FileOpen_Write)
			access |= GENERIC_WRITE;

		DWORD dispositions = OPEN_ALWAYS;
		if (opts & FileOpen_Exists)
			dispositions = OPEN_EXISTING;

		HANDLE fhandle = CreateFile(path, access, 0, nullptr,
			dispositions, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fhandle == INVALID_HANDLE_VALUE)
		{
			cl_system_err("Win32::CreateFile", GetLastError(), path);
			return HFile((uint64)INVALID_HANDLE_VALUE);
		}

		return HFile((uint64)fhandle);
	}

	void file_close(HFile hfile)
	{
		if (hfile.is_invalid())
			return;

		CloseHandle((HANDLE)hfile._v);
	}

	rsize file_write(HFile hfile, const byte* buffer, rsize bytes_to_write)
	{
		if (hfile.is_invalid() || buffer == nullptr || bytes_to_write == 0)
			return 0;

		DWORD written = 0;
		if (!WriteFile((HANDLE)hfile._v, buffer, (DWORD)bytes_to_write, &written, nullptr))
			cl_system_err("Win32::WriteFile", GetLastError(), (uint64&)hfile);

		return (int)written;
	}

	rsize file_read(HFile hfile, byte* buffer, rsize bytes_to_read)
	{
		if (hfile.is_invalid() || buffer == nullptr || bytes_to_read == 0)
			return 0;

		DWORD read = 0;
		if (!ReadFile((HANDLE)hfile._v, buffer, bytes_to_read, &read, nullptr))
			cl_system_err("Win32::ReadFile", GetLastError(), (uint64&)hfile);

		return (int)read;
	}
}
#else
#	error No implementation available for this platform
#endif