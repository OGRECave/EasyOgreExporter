////////////////////////////////////////////////////////////////////////////////
// paramlist.h
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

#ifndef PARAMLIST_H
#define PARAMLIST_H

#include "ExPrerequisites.h"

namespace EasyOgreExporter
{
	class ExSubEntity;

	typedef enum
	{
		TS_TEXCOORD,
		TS_TANGENT
	} TangentSemantic;

	typedef enum
	{
		TOGRE_1_8,
		TOGRE_1_7,
    TOGRE_1_4,
    TOGRE_1_0
	} OgreTarget;

	/***** Class ParamList *****/
	class ParamList
	{
	public:
		// class members
		bool exportMesh, exportMaterial, exportCameras, exportLights, lightingOff, exportAll,
			exportVertNorm, exportVertCol, exportSkeleton, exportSkelAnims, exportVertAnims, exportPoses, 
			useSharedGeom, copyTextures, tangentsSplitMirrored, tangentsSplitRotated, tangentsUseParity, 
			buildTangents, buildEdges, resampleAnims, yUpAxis, exportScene, generateLOD;

		float lum;	// Length Unit Multiplier

		std::string outputDir, meshOutputDir, materialOutputDir, texOutputDir, sceneFilename, matPrefix;

		std::vector<std::string> writtenMaterials;

		TangentSemantic tangentSemantic;

    OgreTarget meshVersion;

		std::vector<INode*> currentRootJoints;

		// constructor
		ParamList()	{
			lum = 1.0f;
			exportMesh = true;
			exportMaterial = true;
			exportSkeleton = true;
			exportSkelAnims = true;
			exportVertAnims = true;
			exportPoses = true;
			exportCameras = true;
      exportLights = true;
			exportAll = true;
			exportVertNorm = true;
			exportVertCol = true;
      lightingOff = false;
			useSharedGeom = false;
			copyTextures = true;

      resampleAnims = false;
      generateLOD = false;

      outputDir = "";
      meshOutputDir = "";
      materialOutputDir = "";
      texOutputDir = "";
      sceneFilename = "";
      matPrefix = "";
		  
			buildEdges = true;
			buildTangents = true;
			tangentsSplitMirrored = false;
			tangentsSplitRotated = false;
			tangentsUseParity = false;
			tangentSemantic = TS_TANGENT;
			currentRootJoints.clear();
			yUpAxis = true;
			exportScene = true;

      meshVersion = TOGRE_1_8;
		}

		ParamList& operator=(ParamList& source)	
		{
			lum = source.lum;
			exportMesh = source.exportMesh;
			exportMaterial = source.exportMaterial;
			exportSkeleton = source.exportSkeleton;
			exportSkelAnims = source.exportSkelAnims;
			exportVertAnims = source.exportVertAnims;
			exportPoses = source.exportPoses;
			exportCameras = source.exportCameras;
      exportLights = source.exportLights;
			exportAll = source.exportAll;
			exportVertNorm = source.exportVertNorm;
			exportVertCol = source.exportVertCol;
			useSharedGeom = source.useSharedGeom;
			copyTextures = source.copyTextures;
      lightingOff = source.lightingOff;

      resampleAnims = source.resampleAnims;
      generateLOD = source.generateLOD;
     
      outputDir = source.outputDir;
			meshOutputDir = source.meshOutputDir;
      materialOutputDir = source.materialOutputDir;
			texOutputDir = source.texOutputDir;
      sceneFilename = source.sceneFilename;
      matPrefix = source.matPrefix;
      			
			buildEdges = source.buildEdges;
			buildTangents = source.buildTangents;
			tangentsSplitMirrored = source.tangentsSplitMirrored;
			tangentsSplitRotated = source.tangentsSplitRotated;
			tangentsUseParity = source.tangentsUseParity;
			tangentSemantic = source.tangentSemantic;
			yUpAxis = source.yUpAxis;
			exportScene = source.exportScene;
      meshVersion = source.meshVersion;

			return *this;
		}

		// destructor
		~ParamList()
    {
		}

    Ogre::MeshVersion getOgreVersion()
    {
      switch(meshVersion)
      {
		    case TOGRE_1_8:
          return Ogre::MESH_VERSION_1_8;

        case TOGRE_1_7:
          return Ogre::MESH_VERSION_1_7;

        case TOGRE_1_4:
          return Ogre::MESH_VERSION_1_4;

        case TOGRE_1_0:
          return Ogre::MESH_VERSION_1_0;

        default:
          return Ogre::MESH_VERSION_LATEST;
      }
    }
	};

};	//end namespace

#endif
