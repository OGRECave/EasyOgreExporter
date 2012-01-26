////////////////////////////////////////////////////////////////////////////////
// ExBlendshape.h
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

#ifndef _EXBLENDSHAPE_H
#define _EXBLENDSHAPE_H

#include "ExPrerequisites.h"
#include "paramList.h"
#include "ExAnimation.h"

namespace EasyOgreExporter
{
	typedef struct
	{
		int targetIndex;
		std::vector<pose> poses;
	} poseGroup;

	// Blend Shape Class
	class ExBlendShape
	{
	public:
		// Constructor
		ExBlendShape(MorphR3* pMorphR3, IGameNode* pGameNode, IGameMesh* pGameMesh, ParamList &params);

		// Destructor
		~ExBlendShape();

		// Clear blend shape data
		void clear();

		// Load blend shape poses
		//bool loadPoses(std::vector<vertex> &vertices, long numVertices,long offset=0,long targetIndex=0);

		//load a blend shape animation track
		ExTrack loadTrack(float start,float stop,float rate,int targetIndex, int startPoseId);
		
    // Get blend shape deformer name
		std::string getName();
		
    // Get blend shape poses
		stdext::hash_map<int, poseGroup>& getPoseGroups();

	protected:
		// Internal methods

		// Protected members
		IGameMesh* m_pGameMesh;
    IGameNode* m_pGameNode;
		MorphR3 *m_pMorphR3;
    ParamList m_params;

		//original values to restore after export
		float m_origEnvelope;
		std::vector<float> m_origWeights;

		//blend shape poses
		stdext::hash_map<int, poseGroup> m_poseGroups;
		
    // An array of morphChannel* to correspond to each pose in m_poses 
		std::vector<morphChannel*> m_posesChannels;
		
    //blend shape target (shared geometry or submesh)
		target m_target;
	};


}	// end namespace

#endif
