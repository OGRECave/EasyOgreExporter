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

	// Skeleton animation keyframe
	typedef struct skeletonKeyframeTag
	{
		float time;							//time of keyframe
		Point3 trans;						//translation
		Quat rot;		            //rotation
		Point3 scale;					  //scale
    Point3 bbpos;					  //pos for bb
	} skeletonKeyframe;

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
			m_index = 0;
			m_bone = "";
			m_skeletonKeyframes.clear();
		}

		//add skeleton animation keyframe
		void addSkeletonKeyframe(skeletonKeyframe& k)
    {
			m_skeletonKeyframes.push_back(k);
		}

    void optimize()
    {
      if (m_skeletonKeyframes.size() > 2)
      {
        skeletonKeyframe prevKey;
        skeletonKeyframe nextKey;
        skeletonKeyframe key;
        for (int i = 0; i < m_skeletonKeyframes.size(); i++)
        {
          if (m_skeletonKeyframes.size() > i + 2)
          {
            prevKey = m_skeletonKeyframes[i];
            nextKey = m_skeletonKeyframes[i + 1];
            key = m_skeletonKeyframes[i + 2];

            if(key.rot.Equals(prevKey.rot) != 0 && key.rot.Equals(nextKey.rot) != 0 &&
               key.trans.Equals(prevKey.trans) != 0 && key.trans.Equals(nextKey.trans) != 0 &&
               key.scale.Equals(prevKey.scale) != 0 && key.scale.Equals(nextKey.scale) != 0)
            {
              //remove the key
              m_skeletonKeyframes.erase(m_skeletonKeyframes.begin() + (i + 1));
              i--;
            }
          }
        }
      }
    }

		//public members
		trackType m_type;
		int m_index;
		std::string m_bone;
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
