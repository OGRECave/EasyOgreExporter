////////////////////////////////////////////////////////////////////////////////
// ExAnimation.h
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

#ifndef EXANIMATION_H
#define EXANIMATION_H

#include "ExPrerequisites.h"

namespace EasyOgreExporter
{
	// Track type
	typedef enum { TT_SKELETON, TT_MORPH, TT_POSE } trackType;

	// Target
	typedef enum { T_MESH, T_SUBMESH } target;

	// Vertex position
	typedef struct vertexPositiontag
	{
		float x,y,z;
	} vertexPosition;

	// Vertex pose reference
	typedef struct vertexPoseReftag
	{
		int poseIndex;
		float poseWeight;
	} vertexPoseRef;

	// Vertex animation keyframe
	typedef struct vertexKeyframetag
	{
		float time;
		std::vector<vertexPosition> positions;
		std::vector<vertexPoseRef> poserefs;
	} vertexKeyframe;

	// Skeleton animation keyframe
	typedef struct skeletonKeyframeTag
	{
		float time;							//time of keyframe
		Point3 trans;						//translation
		Quat rot;		            //rotation
		Point3 scale;					  //scale
	} skeletonKeyframe;

	// Blend shape data
	typedef struct vertexOffestTag
	{
		long index;
		float x,y,z;
	} vertexOffset;

	typedef struct poseTag
	{
		target poseTarget;
		long index;
		int blendShapeIndex;
		std::string name;
		std::vector<vertexOffset> offsets;
		morphChannel* pMChannel;
	} pose;

	// A class for storing an animation track
	// each track can be either skeleton, morph or pose animation
	class ExTrack
	{
	public:
		//constructor
		ExTrack()
    {
			clear();
		}

		//destructor
		~ExTrack()
    {
			clear();
		}

		//clear track data
		void clear()
    {
			m_type = TT_SKELETON;
			m_target = T_MESH;
			m_index = 0;
			m_bone = "";
			m_vertexKeyframes.clear();
			m_skeletonKeyframes.clear();
		}

		//add vertex animation keyframe
		void addVertexKeyframe(vertexKeyframe& k)
    {
			m_vertexKeyframes.push_back(k);
		}

		//add skeleton animation keyframe
		void addSkeletonKeyframe(skeletonKeyframe& k)
    {
			m_skeletonKeyframes.push_back(k);
		}

		//public members
		trackType m_type;
		target m_target;
		int m_index;
		std::string m_bone;
		std::vector<vertexKeyframe> m_vertexKeyframes;
		std::vector<skeletonKeyframe> m_skeletonKeyframes;
	};
	

	// A class for storing animation information
	// an animation is a collection of different tracks
	class ExAnimation
	{
	public:
		//constructor
		ExAnimation()
    {
			clear();
		}

		//destructor
		~ExAnimation()
    { 
			clear(); 
		}

		//clear animation data
		void clear()
    {
			m_name = "";
			m_length = 0;
			m_tracks.clear();
		}

		//add track
		void addTrack(ExTrack& t)
    {
			m_tracks.push_back(t);
		}

		//public memebers
		std::string m_name;
		float m_length;
		std::vector<ExTrack> m_tracks;
	};

} // end namespace


#endif // ANIMATION_H
