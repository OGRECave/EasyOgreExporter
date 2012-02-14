////////////////////////////////////////////////////////////////////////////////
// ExPrerequisites.h
// Author   : Bastien BOURINEAU
// Start Date : January 21, 2012
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                        *
*   This program is free software; you can redistribute it and/or modify     *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or      *
*   (at your option) any later version.                      *
*                                        *
**********************************************************************************/
////////////////////////////////////////////////////////////////////////////////
// Port to 3D Studio Max - Modified original version
// Author	      : Doug Perkowski - OC3 Entertainment, Inc.
// From work of : Francesco Giordana
// Start Date   : December 10th, 2007
////////////////////////////////////////////////////////////////////////////////

#ifndef _EXPREREQUISITES_H
#define _EXPREREQUISITES_H

#define PRECISION 0.000001

#include "max.h"
#include "iparamb2.h"
#include "iparamm2.h"

#include <IGame.h>
#include <IGameModifier.h>
#include <IPathConfigMgr.h> 
#include <ILayerControl.h> 

#ifdef PRE_MAX_2011
#include "maxscrpt/maxscrpt.h"
#else
#include "maxscript/maxscript.h"
#endif

// This file is not included in the max SDK directly, but in the morpher sample.
// It is required to get access to the Max Morpher. 
#include "wm3.h"

#pragma warning (disable : 4996)
#pragma warning (disable : 4267)
#pragma warning (disable : 4018)

// These come from the resource file included with wm3.h.
#define IDS_CLASS_NAME                  102
#define IDS_MORPHMTL                    39
#define IDS_MTL_MAPNAME                 45
#define IDS_MTL_BASENAME                46

// OGRE API
// Max defines PI and OgreMath.h fails to compile as a result.
#undef PI 
#include "Ogre.h"

// This used to be contained in a file called OgreNoMemoryMacros.h, which was removed in version 1.6 of Ogre.
#ifdef OGRE_MEMORY_MACROS
#undef OGRE_MEMORY_MACROS
#undef new
#undef delete
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif

#include "OgreDefaultHardwareBufferManager.h"
#define PI 3.1415926535f



// standard libraries
#include <math.h>
#include <vector>
#include <set>
#include <cassert>

extern TCHAR* GetString(int id);
extern HINSTANCE hInstance;

#endif
