////////////////////////////////////////////////////////////////////////////////
// ExOgreConverter.h
// Author     : Bastien Bourineau
// Start Date : Junary 11th, 2012
// Copyright  : Copyright (c) 2011 OpenSpace3D
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
#ifndef _EXOGRECONVERTER_H
#define _EXOGRECONVERTER_H


#include "paramList.h"
#include "materialSet.h"

namespace EasyOgreExporter
{
	class ExOgreConverter
	{
	  public:
		  //constructor
		  ExOgreConverter(ParamList &params);

		  //destructor
		  ~ExOgreConverter();

		  bool writeEntityData(IGameNode* pGameNode, IGameObject* pGameObject, IGameMesh* pGameMesh);
      bool writeMaterialFile();
      MaterialSet* getMaterialSet();

	  protected:
      ParamList mParams;
      MaterialSet* mMaterialSet;
	};

}; // end of namespace

#endif
