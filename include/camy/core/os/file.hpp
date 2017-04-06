/* file.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>

/*!
	Simple filesystem interface. Nothing fancy.
!*/
namespace camy
{
	enum FileOpen
	{
		//! Reading 
		FileOpen_Read = 1,

		//! Writing
		FileOpen_Write = 1 << 1,

		//! Fails if file does not exist
		FileOpen_Exists = 1 << 2
	};

	//! Offset bytes are offset from when reading/writing
	enum class FileSeekOff
	{
		Start,
		Cur,
		End
	};

	//! Timestamp
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

	//! File info
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
		//! Opaque, probably a system handle, do not use this
		uint64 _v;

		//! Creates a new invalid file handle
		HFile(uint64);

		//! True if valid
		bool is_valid()const;

		//! True if invalid
		bool is_invalid()const;
	};
#pragma pack(pop)

	//! Checks whether the specified file exists and is supported
	bool file_exists(const char8* path);

	//! Deletes the file. No warning is issued if the file does not exist.
	void  file_delete(const char8* path);

	//! Retrieves useful information about the file w/o opening it
	bool  file_get_info(const char8* path, FileInfo& info);

	//! Opens a file for subsequent operations as specified by opts
	HFile file_open(const char8* path, int opts);

	//! Closes a previously opened file handle. No warning issued if handle is invalid
	void  file_close(HFile hfile);
	
	//! Writes bytes_to_write from buffer into a valid HFile, returns the number of bytes writtern
	rsize file_write(HFile hfile, const byte* buffer, rsize bytes_to_write);

	//! Reads bytes_to_read into buffer from a valid HFile, returns the number of bytes read
	rsize file_read(HFile hfile, byte* buffer, rsize bytes_to_read);

	//! Path utility that returns the filename given the path.
	//! the pointer returned is part of the same input buffer
	//! This is a very trivial implementation, extracting a path is very straightforward
	//! 99% of the cases and that's exactly what's covered.
	//! Both forward and backward slash separators are supported
	//! Device specification as in Windows is supported aswell
	//! UNC works out of the box as it is treated as a relative path
	//! Note: Assuming the string is null-terminated
	//! Example: C:\\Users\\test\\Documents\\file.obj.ext returns file.obj.ext
	//! As the buffer returns is simply an incremented pointer
	const char8* path_extract_filename(const char8* path);
}