////////////////////////////////////////////////////////////////////////////////
// ExOgreConverter.cpp
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
#include "ExOgreConverter.h"
#include "EasyOgreExporterLog.h"
#include "ExMesh.h"
#include "decomp.h"

namespace EasyOgreExporter
{
	// constructor
	ExOgreConverter::ExOgreConverter(ParamList &params)
	{
    mParams = params;
    mMaterialSet = new MaterialSet();
	}

	// destructor
	ExOgreConverter::~ExOgreConverter()
	{
    if(mMaterialSet)
    {
      delete mMaterialSet;
      mMaterialSet = 0;
    }
	}
  
  MaterialSet* ExOgreConverter::getMaterialSet()
  {
    return mMaterialSet;
  }

  bool ExOgreConverter::writeEntityData(IGameNode* pGameNode, IGameObject* pGameObject, IGameMesh* pGameMesh)
  {
    bool ret = false;
    ExMesh* mesh = new ExMesh(this, mParams, pGameNode, pGameMesh, pGameNode->GetName());

    if (mParams.exportMesh)
    {
      EasyOgreExporterLog("Writing %s mesh binary...\n", pGameNode->GetName());
      ret = mesh->writeOgreBinary();
      if (ret != true)
      {
        EasyOgreExporterLog("Error writing mesh binary file\n");
      }
    }

    // Write skeleton binary
    if (mParams.exportSkeleton && mesh->getSkeleton())
    {
      // Restore skeleton to correct pose
      mesh->getSkeleton()->restorePose();
      // Load skeleton animations
      mesh->getSkeleton()->loadAnims(pGameNode);
      
      EasyOgreExporterLog("Writing skeleton binary...\n");
      if(!mesh->getSkeleton()->writeOgreBinary())
      {
        EasyOgreExporterLog("Error writing mesh binary file\n");
      }
    }

    //TODO
    /*
    // Load vertex animations
    if (mParams.exportVertAnims)
      mesh->loadAnims(mParams);

    // Load blend shapes
    if (mParams.exportBlendShapes)
      mesh->loadBlendShapes(mParams);
    */

    delete mesh;
    return ret;
  }

  bool ExOgreConverter::writeMaterialFile()
  {
    bool ret = true;
    if (mParams.exportMaterial)
    {
      EasyOgreExporterLog("Writing materials data...\n");
      if (!mMaterialSet->writeOgreScript(mParams))
      {
        EasyOgreExporterLog("Error writing materials file\n");
        ret = false;
      }
    }

    return ret;
  }

}; //end of namespace
