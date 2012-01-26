////////////////////////////////////////////////////////////////////////////////
// EasyOgreExporterMaxScript.h
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

#ifndef EasyOgreExporterMaxScript_H__
#define EasyOgreExporterMaxScript_H__

#include "ExPrerequisites.h"
#include "ExData.h"


class EasyOgreMaxScriptInterfaceClassDesc : public ClassDesc2 
{
public:
	// Controls if the plug-in shows up in lists from the user to choose from.
	int IsPublic(void);
	// Max calls this method when it needs a pointer to a new instance of the 
	// plug-in class.
	void* Create(BOOL loading = FALSE);
	// Returns the name of the class.
	const TCHAR* ClassName(void);
	// Returns a system defined constant describing the class this plug-in 
	// class was derived from.
	SClass_ID SuperClassID(void);
	// Returns the unique ID for the object.
	Class_ID ClassID(void);
	// Returns a string describing the category the plug-in fits into.
	const TCHAR* Category(void);

	// Returns a string which provides a fixed, machine parsable internal name 
	// for the plug-in.  This name is used by MAXScript.
	const TCHAR* InternalName(void);
	// Returns the DLL instance handle of the plug-in.
	HINSTANCE HInstance(void);	
};

#endif 
