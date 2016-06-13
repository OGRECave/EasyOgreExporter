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
#include "ExMaterialSet.h"

namespace EasyOgreExporter
{
	class ExOgreConverter
	{
	  public:
      IGameScene* pIGame;

		  //constructor
		  ExOgreConverter(IGameScene* pIGameScene, ParamList params);

		  //destructor
		  ~ExOgreConverter();

      // for skeleton unicity
      void addExportedRootBone(ExBone bone);
      bool isExportedRootBone(ExBone bone);
      void addSkinModifier(IGameSkin* skinmod, IGameNode* node);
      void setAllSkinToBindPos();
      void restoreAllSkin();
      void setHasError(bool state);
      bool hasError();

		  bool writeEntityData(IGameNode* pGameNode, IGameObject* pGameObject, IGameMesh* pGameMesh, std::vector<ExMaterial*>& lmat);
      bool writeMaterialFile();
      ExMaterialSet* getMaterialSet();
      ParamList getParams();

	  private:
      ParamList mParams;
      ExMaterialSet* mMaterialSet;
      std::vector<ExBone> mExportedRootBones;
      std::vector<Modifier*> mSkinList;
      std::vector<INode*> mSkinNodeList;
      std::vector<DWORD> mSkinLastStateList;
      bool mHasError;
	};

}; // end of namespace

#endif
