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

// Length units multipliers from Maya internal unit (cm)

#define CM2MM 10.0
#define CM2CM 1.0
#define CM2M  0.01
#define CM2IN 0.393701
#define CM2FT 0.0328084
#define CM2YD 0.0109361


namespace EasyOgreExporter
{
	std::string StripToTopParent(const std::string& filepath);
  std::string makeOutputPath(std::string common, std::string dir, std::string file, std::string ext);

	class ExSubEntity;

	typedef struct clipInfoTag
	{
		float start;							//start time of the clip
		float stop;								//end time of the clip
		float rate;								//sample rate of anim curves, -1 means auto
		std::string name;				  //clip name
	} clipInfo;

	typedef enum
	{
		NPT_CURFRAME,
		NPT_BINDPOSE
	} NeutralPoseType;

	typedef enum
	{
		TS_TEXCOORD,
		TS_TANGENT
	} TangentSemantic;

	/***** Class ParamList *****/
	class ParamList
	{
	public:
		// class members
		bool exportMesh, exportMaterial, exportAnimCurves, exportCameras, exportAll, exportVBA,
			exportVertNorm, exportVertCol, exportTexCoord, exportCamerasAnim,
			exportSkeleton, exportSkelAnims, exportBSAnims, exportVertAnims, exportBlendShapes, 
			useSharedGeom, lightingOff, copyTextures, exportParticles,
			tangentsSplitMirrored, tangentsSplitRotated, tangentsUseParity, 
			buildTangents, buildEdges, skelBB, bsBB, vertBB, normalizeScale, yUpAxis, exportScene;

		float lum;	// Length Unit Multiplier

		std::string outputDir, meshOutputDir, materialOutputDir, texOutputDir, particlesOutputDir, sceneFilename, matPrefix;

		std::ofstream outParticles;

		std::vector<std::string> writtenMaterials;

		std::vector<clipInfo> skelClipList;
		std::vector<clipInfo> BSClipList;
		std::vector<clipInfo> vertClipList;

		NeutralPoseType neutralPoseType;
		TangentSemantic tangentSemantic;

		std::vector<INode*> currentRootJoints;

		// constructor
		ParamList()	{
			lum = CM2CM;
			exportMesh = true;
			exportMaterial = true;
			exportSkeleton = true;
			exportSkelAnims = true;
			exportBSAnims = false;
			exportVertAnims = false;
			exportBlendShapes = false;
			exportAnimCurves = false;
			exportCameras = false;
			exportParticles = false;
			exportAll = true;
			exportVBA = false;
			exportVertNorm = true;
			exportVertCol = true;
			exportCamerasAnim = false;
			useSharedGeom = false;
			lightingOff = false;
			copyTextures = true;
			skelBB = false;
			bsBB = false;
			vertBB = false;

      outputDir = "";
      meshOutputDir = "";
      materialOutputDir = "";
      texOutputDir = "";
      particlesOutputDir = "";
      sceneFilename = "";
      matPrefix = "";
					
      skelClipList.clear();
			BSClipList.clear();
			vertClipList.clear();
			neutralPoseType = NPT_CURFRAME;
			buildEdges = true;
			buildTangents = true;
			tangentsSplitMirrored = false;
			tangentsSplitRotated = false;
			tangentsUseParity = false;
			tangentSemantic = TS_TANGENT;
			currentRootJoints.clear();
			normalizeScale = true; // Remove's scale from animation (to solve for Max's unorthodox use of non-uniform scale)
			yUpAxis = true;
			exportScene = true;
		}

		ParamList& operator=(ParamList& source)	
		{
			int i;
			lum = source.lum;
			exportMesh = source.exportMesh;
			exportMaterial = source.exportMaterial;
			exportSkeleton = source.exportSkeleton;
			exportSkelAnims = source.exportSkelAnims;
			exportBSAnims = source.exportBSAnims;
			exportVertAnims = source.exportVertAnims;
			exportBlendShapes = source.exportBlendShapes;
			exportAnimCurves = source.exportAnimCurves;
			exportCameras = source.exportCameras;
			exportAll = source.exportAll;
			exportVBA = source.exportVBA;
			exportVertNorm = source.exportVertNorm;
			exportVertCol = source.exportVertCol;
			exportCamerasAnim = source.exportCamerasAnim;
			exportParticles = source.exportParticles;
			useSharedGeom = source.useSharedGeom;
			lightingOff = source.lightingOff;
			copyTextures = source.copyTextures;
			skelBB = source.skelBB;
			bsBB = source.bsBB;
			vertBB = source.vertBB;

      outputDir = source.outputDir;
			meshOutputDir = source.meshOutputDir;
      materialOutputDir = source.materialOutputDir;
			texOutputDir = source.texOutputDir;
      particlesOutputDir = source.particlesOutputDir;
      sceneFilename = source.sceneFilename;
      matPrefix = source.matPrefix;
      			
			buildEdges = source.buildEdges;
			buildTangents = source.buildTangents;
			tangentsSplitMirrored = source.tangentsSplitMirrored;
			tangentsSplitRotated = source.tangentsSplitRotated;
			tangentsUseParity = source.tangentsUseParity;
			tangentSemantic = source.tangentSemantic;
			skelClipList.resize(source.skelClipList.size());
			normalizeScale = source.normalizeScale;
			yUpAxis = source.yUpAxis;
			exportScene = source.exportScene;

			for (i=0; i< skelClipList.size(); i++)
			{
				skelClipList[i].name = source.skelClipList[i].name;
				skelClipList[i].start = source.skelClipList[i].start;
				skelClipList[i].stop = source.skelClipList[i].stop;
				skelClipList[i].rate = source.skelClipList[i].rate;
			}
			BSClipList.resize(source.BSClipList.size());
			for (i=0; i< BSClipList.size(); i++)
			{
				BSClipList[i].name = source.BSClipList[i].name;
				BSClipList[i].start = source.BSClipList[i].start;
				BSClipList[i].stop = source.BSClipList[i].stop;
				BSClipList[i].rate = source.BSClipList[i].rate;
			}
			vertClipList.resize(source.vertClipList.size());
			for (i=0; i< vertClipList.size(); i++)
			{
				vertClipList[i].name = source.vertClipList[i].name;
				vertClipList[i].start = source.vertClipList[i].start;
				vertClipList[i].stop = source.vertClipList[i].stop;
				vertClipList[i].rate = source.vertClipList[i].rate;
			}
			neutralPoseType = source.neutralPoseType;

			for (i=0; i<source.currentRootJoints.size(); i++)
				currentRootJoints.push_back(source.currentRootJoints[i]);
			return *this;
		}

		// destructor
		~ParamList() {
			if (outParticles)
				outParticles.close();
		}
		// method to open files for writing
		bool openFiles();
		// method to close open output files
		bool closeFiles();
	};

};	//end namespace

#endif
