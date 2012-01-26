////////////////////////////////////////////////////////////////////////////////
// EasyOgreExporterLog.h
// Author	  : Jamie Redmond - OC3 Entertainment, Inc.
// Copyright  : (C) 2007 OC3 Entertainment, Inc.
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/

#ifndef EasyOgreExporterLog_H__
#define EasyOgreExporterLog_H__

#include <string>

namespace EasyOgreExporter
{

#define EasyOgreExporterLog ::EasyOgreExporter::EasyOgreExporterLogFile::Log

class EasyOgreExporterLogFile
{
public:
	// Sets the full path to the log file.
	static void SetPath(const std::string& logPath);
	// Returns the full path to the log file.
	static std::string GetPath(void);

	// Printf-style log output.
	static void Log(const char* format, ...);

	void operator<<(const char* str);
protected:
	// The full path to the log file.
	static std::string _logPath;
};

} // namespace 

#endif
