////////////////////////////////////////////////////////////////////////////////
// ExSkeleton.h
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

#ifndef _EXSKELETON_H
#define _EXSKELETON_H

#include "ExPrerequisites.h"
#include "paramList.h"
#include "ExAnimation.h"

namespace EasyOgreExporter
{
	/***** structure to hold joint info *****/
	typedef struct jointTag
	{
		std::string name;
		int id;
		ULONG nodeID; //Unique Max INode ID ;
		INode* pNode;
		Matrix3 bindMatrix;
		int parentIndex;
		Point3 trans;
    Point3 scale;
		Quat rot;
	} joint;


	class ExSkeleton
	{
	public:
		//constructor
    ExSkeleton(IGameSkin* pGameSkin, std::string name, ParamList &params);
		//destructor
		~ExSkeleton();
		//clear skeleton data
		void clear();
		//load skeleton data
//		bool load(IGameNode* pGameNode, IGameObject* pGameObject, IGameSkin* pGameSkin);
		
    bool getVertexBoneWeights(IGameMesh* pGameMesh);

		// returns the index of the bone in the skeleton.  -1 if it doesn't exist.
		int getJointIndex(INode* pNode);

		//load skeletal animations
		bool loadAnims(IGameNode* pGameNode);
		//get joints
		std::vector<joint>& getJoints();
		//get animations
		std::vector<ExAnimation>& getAnimations();
		//restore skeleton pose
		void restorePose();
		//write to an OGRE binary skeleton
		bool writeOgreBinary();

    const std::vector<float> getWeightList(int index);
    const std::vector<int> getJointList(int index);

	protected:

		//load a clip
		bool loadClip(std::string clipName, int start, int stop, int rate);
		//load a keyframe for a particular joint at current time
		skeletonKeyframe loadKeyframe(joint& j, int time);
		//write joints to an Ogre skeleton
		bool createOgreBones(Ogre::SkeletonPtr pSkeleton);
		// write skeleton animations to an Ogre skeleton
		bool createOgreSkeletonAnimations(Ogre::SkeletonPtr pSkeleton);
    //load a joint
		bool loadJoint(INode* pNode);

    
    IGameSkin* m_pGameSkin;
		Control* m_bipedControl;
    std::vector<joint> m_joints;
		std::vector<ExAnimation> m_animations;
		std::vector<int> m_roots;
    std::vector< std::vector<float> > m_weights;
		std::vector< std::vector<int> > m_jointIds;
		std::string m_restorePose;
    std::string m_name;
    ParamList m_params;
	};

}	//end namespace

#endif
