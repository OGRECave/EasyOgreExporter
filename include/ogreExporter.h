////////////////////////////////////////////////////////////////////////////////
// ogreExporter.h
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

#ifndef OGRE_EXPORTER_H
#define OGRE_EXPORTER_H

#include "ExScene.h"
#include "ExOgreConverter.h"
#include "ExPrerequisites.h"
namespace EasyOgreExporter
{

class OgreSceneExporter : public SceneExport 
{
public:

	// public methods
	OgreSceneExporter();
	virtual ~OgreSceneExporter();

	int ExtCount(void);
	const TCHAR* Ext(int n);
	const TCHAR* LongDesc(void);
	const TCHAR* ShortDesc(void);
	const TCHAR* AuthorName(void);
	const TCHAR* CopyrightMessage(void);
	const TCHAR* OtherMessage1(void);
	const TCHAR* OtherMessage2(void);
	unsigned int Version(void);
	void ShowAbout(HWND hWnd);
	BOOL SupportsOptions(int ext, DWORD options);
	int	DoExport(const TCHAR* name, ExpInterface* pExpInterface, Interface* pInterface, BOOL suppressPrompts = FALSE, DWORD options = 0);
};

class OgreExporter 
{
public:
	// public methods
	OgreExporter();
	~OgreExporter();

	bool exportScene();	
	ParamList m_params;

private:
  ExScene* sceneData;
  ExOgreConverter* ogreConverter;
	TimeValue m_curTime;
	IGameScene* pIGame;
  
  bool exportNode(IGameNode* pGameNode, TiXmlElement* parent);
};

}	//end namespace

#endif
