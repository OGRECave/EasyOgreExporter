////////////////////////////////////////////////////////////////////////////////
// ExData.h
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

#include "ExPrerequisites.h"
#include "iparamb2.h"

extern ClassDesc2* GetEasyOgreExporterDesc(void);
//extern ClassDesc2* GetEasyOgreMaxScriptInterfaceClassDesc(void);
extern HINSTANCE hInstance;

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
   if(fdwReason == DLL_PROCESS_ATTACH)
   {
	   // Hang on to this DLL's instance handle.
      hInstance = hinstDLL;        
      DisableThreadLibraryCalls(hInstance);
   }
	return TRUE;
}

__declspec(dllexport) const TCHAR* LibDescription(void)
{
	return _T("EasyOgreExporter");
}

__declspec(dllexport) int LibNumberClasses(void)
{
	return 2;
}

__declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
	switch(i) 
	{
		case 0: return GetEasyOgreExporterDesc();
		//case 1 : return GetEasyOgreMaxScriptInterfaceClassDesc();
		default: return 0;
	}
}

__declspec(dllexport) ULONG LibVersion(void)
{
	return VERSION_3DSMAX;
}

__declspec(dllexport) ULONG CanAutoDefer(void)
{
	return 1;
}

TCHAR* GetString(int id)
{
	static TCHAR buf[256];

	if(hInstance)
	{
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	}
	return NULL;
}
