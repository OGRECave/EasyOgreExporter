////////////////////////////////////////////////////////////////////////////////
// ExSkeleton.cpp
// Author   : Bastien BOURINEAU
// Start Date : January 21, 2012
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
////////////////////////////////////////////////////////////////////////////////
// Port to 3D Studio Max - Modified original version
// Author	      : Doug Perkowski - OC3 Entertainment, Inc.
// From work of : Francesco Giordana
// Start Date   : December 10th, 2007
////////////////////////////////////////////////////////////////////////////////

#include "ExTools.h"
#include "ExSkeleton.h"
#include "EasyOgreExporterLog.h"
#include "decomp.h"
#include "BipedApi.h"
#include "IMixer.h"
#include "iskin.h"
#include "IFrameTagManager.h"


namespace EasyOgreExporter
{  
  ExSkeleton::ExSkeleton(IGameSkin* pGameSkin, std::string name, ParamList &params)
	{
		m_joints.clear();
		m_animations.clear();
		m_restorePose = "";
    m_name = name;
    m_pGameSkin = pGameSkin;
    m_params = params;
    m_bipedControl = 0;
    m_isBiped = false;
	}

	ExSkeleton::~ExSkeleton()
	{
		clear();
	}

	// Clear skeleton data
	void ExSkeleton::clear()
	{
		m_joints.clear();
    m_weights.clear();
    m_jointIds.clear();
		m_animations.clear();
		m_restorePose = "";
	}

  const std::vector<float> ExSkeleton::getWeightList(int index)
  {
    return m_weights[index];
  }

  const std::vector<int> ExSkeleton::getJointList(int index)
  {
    return m_jointIds[index];
  }

  // Get vertex bone assignements
  bool ExSkeleton::getVertexBoneWeights(IGameMesh* pGameMesh)
  {
    //NOTE don't use IGameNode here sometimes bones can be a mesh and GetBoneNode return null
    EasyOgreExporterLog("Info : Get vertex bone weight\n");
    
    //init list indices
    int numVertices = pGameMesh->GetNumberOfVerts();
    m_weights.resize(numVertices);
    m_jointIds.resize(numVertices);
  
    std::vector<INode*> rootbones;
  
    //nothing to export
    if(m_pGameSkin->GetTotalBoneCount() <= 0)
    {
      EasyOgreExporterLog("Warning : No assigned bones\n");
      return false;
    }

    IBipedExport* BipIface = 0;
    Control* nodeControl = m_pGameSkin->GetBone(0, false)->GetTMController();
    if ((nodeControl->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) || (nodeControl->ClassID() == BIPBODY_CONTROL_CLASS_ID))
    {
      m_isBiped = true;
    }

    if(m_isBiped)
    {
      for(int i = 0; i < m_pGameSkin->GetTotalBoneCount(); ++i)
      {
        // pass true to only get bones used by vertices
        INode* rootbone = m_pGameSkin->GetBone(i, false);
        if(rootbone)
        {
          while(m_pGameSkin->GetBoneIndex(rootbone->GetParentNode(), false) > -1)
          {
            rootbone = rootbone->GetParentNode();
          }

          bool bNewRootBone = true;
          for(int j = 0; j < rootbones.size(); ++j)
          {
            if(rootbones[j] == rootbone)
            {
              // this bone is already in the list
              bNewRootBone = false;
            }
          }

          if(bNewRootBone)
          {
            EasyOgreExporterLog("Info : Found a root bone : %s\n", rootbone->GetName());
            rootbones.push_back(rootbone);
          }
        }
      }
    }
    else
    {
      for(int i = 0; i < m_pGameSkin->GetTotalBoneCount(); ++i)
      {
        // pass true to only get bones used by vertices
        INode* rootbone = m_pGameSkin->GetBone(i, false);
        if(rootbone)
        {
          while(rootbone->GetParentNode() != GetCOREInterface()->GetRootNode())
          {
            rootbone = rootbone->GetParentNode();
          }

          bool bNewRootBone = true;
          for(int j = 0; j < rootbones.size(); ++j)
          {
            if(rootbones[j] == rootbone)
            {
              // this bone is already in the list
              bNewRootBone = false;
            }
          }

          if(bNewRootBone)
          {
            EasyOgreExporterLog("Info : Found a root bone : %s\n", rootbone->GetName());
            rootbones.push_back(rootbone);
          }
        }
      }
    }

    for(int i = 0; i <rootbones.size(); ++i)
    {
      EasyOgreExporterLog("Exporting root bone: %s\n", rootbones[i]->GetName());
      loadJoint(rootbones[i]);
    }

    int numSkinnedVertices = m_pGameSkin->GetNumOfSkinnedVerts();
    EasyOgreExporterLog("Num. Skinned Vertices: %d\n", numSkinnedVertices);
    std::vector<std::string> lwarnings;
    for(int i = 0; i < numSkinnedVertices; ++i)
    {
      int type = m_pGameSkin->GetVertexType(i);
      // Rigid vertices.
      if(type == IGameSkin::IGAME_RIGID)
      {
        INode* pBoneNode = m_pGameSkin->GetBone(i, 0);
        if(pBoneNode && m_pGameSkin->GetBoneIndex(pBoneNode, false) > -1)
        {
          int boneIndex = getJointIndex(pBoneNode);
          if(boneIndex >= 0)
          {
            m_weights[i].push_back(1.0f);
            m_jointIds[i].push_back(boneIndex);

            if(m_weights[i].size() > 4)
              lwarnings.push_back(pBoneNode->GetName());
          }
        }
      }
      // Blended vertices.
      else
      {
        int numWeights = m_pGameSkin->GetNumberOfBones(i);
        for(int j = 0; j < numWeights; ++j)
        {
          INode* pBoneNode = m_pGameSkin->GetBone(i, j);
          if(pBoneNode && m_pGameSkin->GetBoneIndex(pBoneNode, false) > -1)
          {
            int boneIndex = getJointIndex(pBoneNode);
            if(boneIndex >= 0)
            {
              m_weights[i].push_back(m_pGameSkin->GetWeight(i, j));
              m_jointIds[i].push_back(boneIndex);

              if(m_weights[i].size() > 4)
                lwarnings.push_back(pBoneNode->GetName());
            }
          }
        }
      }
    }

    //sort and remove duplicated entries
    if(lwarnings.size() > 0)
    {
      std::sort(lwarnings.begin(), lwarnings.end());
      lwarnings.erase(std::unique(lwarnings.begin(), lwarnings.end()), lwarnings.end());
    }

    if(lwarnings.size() > 0)
    {
      std::string mess = "Warning : Vertex found with more than 4 weights on :\n";

      for(int i = 0; i < lwarnings.size(); ++i)
        mess.append("skeleton " + m_name + " with bone " + lwarnings[i] + "\n");

      mess.append("This is not compatible with hardware skinning method.");
      EasyOgreExporterLog("Warning : Vertex found with more than 4 weights on :\n%s", mess.c_str());
      MessageBox(GetCOREInterface()->GetMAXHWnd(), _T(mess.c_str()), _T("Warning"), MB_OK);
    }
    lwarnings.clear();

    return true;
  }

	int ExSkeleton::getJointIndex(INode* pNode)
	{
		if(pNode)
		{
		  for (int i=0; i<m_joints.size(); i++)
		  {
			  if (m_joints[i].nodeID == pNode->GetHandle())
				  return i;
		  }
		}
		return -1;
	}

	// Load a joint
	bool ExSkeleton::loadJoint(INode* pNode)
	{
    // node index if already exist
    int boneIndex = getJointIndex(pNode);

    // get parent index
    INode* pNodeParent = pNode->GetParentNode();
		int parentIdx = getJointIndex(pNodeParent);

    // test for supported bone type
    if((!IsPossibleBone(pNode)) || (IsBone(pNode) && (pNode->NumberOfChildren() == 0)))
    {
      EasyOgreExporterLog("Info : %s is not a bone.\n", pNode->GetName());
      return false;
    }

    int firstFrame = GetCOREInterface()->GetAnimRange().Start();

    // initialise joint to avoid bad searchs
		joint newJoint;
		newJoint.pNode = pNode;
		newJoint.name = pNode->GetName();
		newJoint.nodeID = pNode->GetHandle();
		newJoint.id = -1;
		newJoint.parentIndex = parentIdx;

		if(boneIndex == -1)
		{
      bool duplicated = true;
      int dpid = 1;
      while (duplicated)
      {
        bool found = false;

			  // Make sure we don't have a duplicate bone name
        for(size_t i = 0; i < m_joints.size(); ++i)
			  {
				  if(newJoint.name == m_joints[i].name)
				  {
            std::string sid;
            std::stringstream strId;
            strId << dpid;
            sid = strId.str();

            newJoint.name = std::string(pNode->GetName()) + sid;
            found = true;
            dpid++;
				  }
			  }
        duplicated = found;
      }
      
			// If this is a new joint, push one back to the end of the array.
			// Otherwise we still continue in case we had previously thought
			// this bone was a root bone (incorrectly).
			m_joints.push_back(newJoint);
			boneIndex = m_joints.size() - 1;
		}
		else
		{
			bool bShouldReExport = false;
			for(size_t i = 0; i < m_roots.size(); i++)
			{
				if(m_joints[m_roots[i]].pNode == pNode)
				{
          if (m_pGameSkin->GetBoneIndex(pNode->GetParentNode(), false) > -1)
          {
            int newParentIndex = getJointIndex(pNode->GetParentNode());

					  if(-1 != newParentIndex)
					  {
						  bShouldReExport = true;
						  m_roots.erase(m_roots.begin()+i);
						  i--;
					  }
          }
				}
			}
			if(!bShouldReExport)
			{
				// no sense in going further as we've already exported this joint and
				// it hasn't changed from a root to a non-root.
				return true;
			}
		}

    // set the new bone index
    m_joints[boneIndex].id = boneIndex;

    // get the biped controller
    if(!m_bipedControl)
    {
      Control* nodeControl = pNode->GetTMController();
      if (nodeControl->ClassID() == BIPBODY_CONTROL_CLASS_ID)
        m_bipedControl = nodeControl;
    }

    DWORD actMode = 0;
    IBipMaster* bipMaster = 0;
    if(m_bipedControl)
    {
      //Get the Biped master Interface from the controller
      bipMaster = (IBipMaster*) m_bipedControl->GetInterface(I_BIPMASTER);
      actMode = bipMaster->GetActiveModes();
      bipMaster->EndModes(actMode, 0);
      bipMaster->BeginModes(BMODE_FIGURE, 0);
    }

    // Get mesh matrix at initial pose
    Modifier* skinMod = m_pGameSkin->GetMaxModifier();
    ISkin* pskin = (ISkin*)skinMod->GetInterface(I_SKIN);
    
    GMatrix GSkinTM;
    m_pGameSkin->GetInitSkinTM(GSkinTM);
    Matrix3 SkinTM = GSkinTM.ExtractMatrix3();

		Matrix3 boneTM;
		Matrix3 ParentTM;

    if(!pskin || pskin->GetBoneInitTM(pNode, boneTM, false) == SKIN_INVALID_NODE_PTR)
      boneTM = pNode->GetNodeTM(firstFrame);

    if(!pskin || pskin->GetBoneInitTM(pNodeParent, ParentTM, false) == SKIN_INVALID_NODE_PTR)
      ParentTM = pNodeParent->GetNodeTM(firstFrame);

    Matrix3 localTM;
		if(parentIdx >= 0)
		{
      localTM = GetRelativeUniformMatrix(boneTM, ParentTM, m_params.yUpAxis);
		}
    else // for root bone use the mesh
    {
      localTM = GetRelativeUniformMatrix(boneTM, ParentTM, m_params.yUpAxis) * Inverse(SkinTM);
    }

    AffineParts ap;
		decomp_affine(localTM, &ap);

    Point3 trans = ap.t * m_params.lum;
    Point3 scale = ap.k;
    Quat rot = ap.q;
    // Notice that in Max we flip the w-component of the quaternion;
    rot.w = -rot.w;

		//if(boneIndex == m_joints.size() - 1)
		//	EasyOgreExporterLog("Exporting joint %s. Trans(%f,%f,%f) Rot(%f,%f,%f,%f), Scale(%f,%f,%f).\n", pNode->GetName(), trans.x, trans.y, trans.z, rot.w, rot.x, rot.y, rot.z, scale.x, scale.y, scale.z);

		// Set joint coords
		m_joints[boneIndex].bindMatrix = localTM;
		m_joints[boneIndex].trans = trans;
    m_joints[boneIndex].scale = scale;
		m_joints[boneIndex].rot = rot;

		// If root is a root joint, save it's index in the roots list
		if (parentIdx < 0)
			m_roots.push_back(m_joints.size() - 1);

    //release max interface
    if(bipMaster)
    {
      bipMaster->EndModes(BMODE_FIGURE, 0);
      bipMaster->BeginModes(actMode, 0);

      m_bipedControl->ReleaseInterface(I_BIPMASTER, bipMaster);
    }

		// Load child joints
    for (size_t i = 0; i < pNode->NumberOfChildren(); i++)
		{
			INode* pChildNode = pNode->GetChildNode(i);
			if(pChildNode)
				loadJoint(pChildNode);
		}

		return true;
	}

	// Load animations
	bool ExSkeleton::loadAnims(IGameNode* pGameNode)
	{
		EasyOgreExporterLog("Loading joint animations...\n");
		
		// clear animations list
		m_animations.clear();

    //load clips from mixer
    IMixer* mixer = 0;
    IBipMaster* bipMaster = 0;
    if(m_bipedControl)
    {
      //Get the Biped master Interface from the controller
      bipMaster = (IBipMaster*) m_bipedControl->GetInterface(I_BIPMASTER);
      if(bipMaster)
        mixer = bipMaster->GetMixer();
    }

    bool useDefault = true;
    //we found a mixer try to load clips
    if(mixer)
    {
      DWORD actMode = bipMaster->GetActiveModes();
      if(!(actMode & BMODE_MIXER))
        bipMaster->BeginModes(BMODE_MIXER, 0);
      
      int clipId = 0;
      int numGroups = mixer->NumTrackgroups();
      for (size_t j = 0; j < numGroups; j++)
      {
        IMXtrackgroup* group = mixer->GetTrackgroup(j);
        EasyOgreExporterLog("Info : mixer track found %s\n", group->GetName());

        int numTracks = group->NumTracks();
        for (size_t k = 0; k < numTracks; k++)
        {
          IMXtrack* track = group->GetTrack(k);
          BOOL tMode = track->GetSolo();
          track->SetSolo(TRUE);

          int numClips = track->NumClips(BOT_ROW);
          for (size_t l = 0; l < numClips; l++)
          {
            IMXclip* clip = track->GetClip(l, BOT_ROW);
            if(clip)
            {
              int start;
              int stop;
              #ifdef PRE_MAX_2010
                std::string clipName = formatClipName(std::string(clip->GetFilename()), clipId);
              #else
              MaxSDK::AssetManagement::AssetUser &clipFile = const_cast<MaxSDK::AssetManagement::AssetUser&>(clip->GetFile());
                std::string clipName = formatClipName(std::string(clipFile.GetFileName()), clipId);
              #endif

              clip->GetGlobalBounds(&start, &stop);
              EasyOgreExporterLog("Info : mixer clip found %s from %i to %i\n", clipName.c_str(), start, stop);
              
              if(loadClip(clipName, start, stop, GetTicksPerFrame()))
                useDefault = false;
              clipId++;
            }
          }
          track->SetSolo(tMode);
        }
      }

      if(!(actMode & BMODE_MIXER))
        bipMaster->EndModes(BMODE_MIXER, 0);
    }

    //release max interface
    if(bipMaster)
      m_bipedControl->ReleaseInterface(I_BIPMASTER, bipMaster);

    //load main clip on actual timeline;
    if(useDefault)
    {
      Interval animRange = GetCOREInterface()->GetAnimRange();
      IFrameTagManager* frameTagMgr = static_cast<IFrameTagManager*>(GetCOREInterface(FRAMETAGMANAGER_INTERFACE));
      int cnt = frameTagMgr->GetTagCount();

      if(!cnt)
      {
        loadClip("default_skl", animRange.Start(), animRange.End(), GetTicksPerFrame());
      }
      else
      {
        for(int i = 0; i < cnt; i++)
        {
          DWORD t = frameTagMgr->GetTagID(i);
          DWORD tlock = frameTagMgr->GetLockIDByID(t);
          
          //ignore locked tags used for animation end
          if(tlock != 0)
            continue;

          TimeValue tv = frameTagMgr->GetTimeByID(t, FALSE);
          TimeValue te = animRange.End();
          
          DWORD tnext = 0;
          if((i + 1) < cnt)
          {
            tnext = frameTagMgr->GetTagID(i + 1);
            te = frameTagMgr->GetTimeByID(tnext, FALSE);
          }

          Interval ianim(tv, te);
          loadClip(std::string(frameTagMgr->GetNameByID(t)), ianim.Start(), ianim.End(), GetTicksPerFrame());
        }
      }
    }

		return true;
	}

	// Load an animation clip
	bool ExSkeleton::loadClip(std::string clipName, int start, int stop, int rate)
	{
		// if skeleton has no joints we can't load the clip
		if (m_joints.size() < 0)
			return false;

		// display clip name
		EasyOgreExporterLog("clip \"%s\"\n", clipName.c_str());
		
		// calculate times from clip sample rate
		if (rate <= 0)
		{
			EasyOgreExporterLog("invalid sample rate for the clip (must be >0), we skip it\n");
			return false;
		}

    //add time steps
    std::vector<int> times;
    times.clear();
		for (float t = start; t < stop; t += rate)
			times.push_back(t);

    //force the last key
		times.push_back(stop);
    times.erase(std::unique(times.begin(), times.end()), times.end());

		// get animation length
		int length = 0;
		if (times.size() >= 0)
			length = times[times.size()-1] - times[0];
		if (length < 0)
		{
			EasyOgreExporterLog("invalid time range for the clip, we skip it\n");
			return false;
		}

		// create the animation
		ExAnimation a;
		a.m_name = clipName.c_str();
		a.m_tracks.clear();
    a.m_length = (static_cast<float>(length) / static_cast<float>(rate)) / GetFrameRate();
		m_animations.push_back(a);
		int animIdx = m_animations.size() - 1;

		// create a track for current clip for all joints
		std::vector<ExTrack> animTracks;
		for (size_t i = 0; i < m_joints.size(); i++)
		{
			ExTrack t;
			t.m_type = TT_SKELETON;
			t.m_bone = m_joints[i].name;
			t.m_skeletonKeyframes.clear();
			animTracks.push_back(t);
		}

		// evaluate animation curves at selected times
		for (size_t i = 0; i < times.size(); i++)
		{
			//int closestFrame = (int)(.5f + times[i]* GetFrameRate());
		
      //set time to wanted sample time
			//GetCOREInterface()->SetTime(closestFrame * GetTicksPerFrame());

      //load a keyframe for every joint at current time
			for (size_t j = 0; j < m_joints.size(); j++)
			{
				skeletonKeyframe key = loadKeyframe(m_joints[j], times[i]);
        
        //EasyOgreExporterLog("add key frame: %f\n", key.time);
        //set key time
        key.time = (static_cast<float>((times[i] - times[0])) / static_cast<float>(rate)) / GetFrameRate();

				//add keyframe to joint track
				animTracks[j].addSkeletonKeyframe(key);
			}

      //TODO
			/*if (params.skelBB)
			{
				// Update bounding boxes of loaded submeshes
				for (j=0; j<params.loadedSubmeshes.size(); j++)
				{
					IGameNode *pGameNode = params.loadedSubmeshes[j]->m_pGameNode;
					if(pGameNode)
					{
						IGameObject* pGameObject = pGameNode->GetIGameObject();
						if(pGameObject)
						{
              //TODO calc with vertices
							Box3 bbox;
							pGameObject->GetBoundingBox(bbox);
							params.loadedSubmeshes[j]->m_boundingBox += bbox;
						}
						pGameNode->ReleaseIGameObject();
					}
				}
			}*/
		}
		// add created tracks to current clip
		for (size_t i = 0; i < animTracks.size(); i++)
		{
			m_animations[animIdx].addTrack(animTracks[i]);
		}

		// display info
		EasyOgreExporterLog("length: %f\n", m_animations[animIdx].m_length);
    if(animTracks.size() > 0)
		  EasyOgreExporterLog("num keyframes: %d\n", animTracks[0].m_skeletonKeyframes.size());
		
		// clip successfully loaded
		return true;
	}

	// Load a keyframe for a given joint at current time
	skeletonKeyframe ExSkeleton::loadKeyframe(joint& j, int time)
	{
    INode* bone = j.pNode;
    
		// Get the bone local matrix for this key
    Matrix3 boneTM = GetRelativeUniformMatrix(bone, time, m_params.yUpAxis);

    GMatrix GSkinTM;
    m_pGameSkin->GetInitSkinTM(GSkinTM);
    Matrix3 SkinTM = GSkinTM.ExtractMatrix3();

    // for root bone use the mesh
    if(j.parentIndex < 0)
      boneTM = boneTM * Inverse(SkinTM);

    Matrix3 relMat = boneTM * Inverse(j.bindMatrix);

    AffineParts ap;
		decomp_affine(relMat, &ap);

    Point3 trans = ap.t * m_params.lum;
    Point3 scale = ap.k;
    Quat rot = ap.q;
    // Notice that in Max we flip the w-component of the quaternion;
    rot.w = -rot.w;

		//create keyframe
		skeletonKeyframe key;
		key.time = 0;

    // don't know why root translation are Z X reverted
    // Damn is there another stupid cases ?
    // what matrix should I use for translations ?
    if(IsBipedRoot(bone))
    {
      key.trans.x = -trans.z;
      key.trans.y = trans.y;
      key.trans.z = trans.x;
    }
    else if(IsBipedRoot(bone->GetParentNode()))
    {
      key.trans.x = trans.x;
      key.trans.y = -trans.z;
      key.trans.z = trans.y;
    }
    else
    {
      // don't know why translation are reverted
		  key.trans.x = trans.z;
      key.trans.y = trans.x;
      key.trans.z = trans.y;
    }

		key.rot = rot;
		key.scale = scale;
		return key;
	}

	// Restore skeleton pose
	void ExSkeleton::restorePose()
	{
		// TODO: required in Max?
	}

	// Get joint list
	std::vector<joint>& ExSkeleton::getJoints()
	{
		return m_joints;
	}

	// Get animations
	std::vector<ExAnimation>& ExSkeleton::getAnimations()
	{
		return m_animations;
	}

	// Write to an OGRE binary skeleton
	bool ExSkeleton::writeOgreBinary()
	{
		// Construct skeleton
		Ogre::SkeletonPtr pSkeleton = Ogre::SkeletonManager::getSingleton().create(m_name.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
    // Create skeleton bones
		if (!createOgreBones(pSkeleton))
		{
			EasyOgreExporterLog("Error writing skeleton binary file\n");
		}

    pSkeleton->setBindingPose();

		// Create skeleton animation
		if (m_params.exportSkelAnims)
		{
			if (!createOgreSkeletonAnimations(pSkeleton))
			{
				EasyOgreExporterLog("Error writing ogre skeleton animations\n");
			}
      else
      {
        // Optimise animations
		    pSkeleton->optimiseAllAnimations();
      }
		}

		// Export skeleton binary
		Ogre::SkeletonSerializer serializer;

    std::string filePath = makeOutputPath(m_params.outputDir, m_params.meshOutputDir, m_name, "skeleton");
		serializer.exportSkeleton(pSkeleton.getPointer(), filePath.c_str(), m_params.getSkeletonVersion());
		pSkeleton.setNull();

		// Skeleton successfully exported
		return true;
	}

	// Write joints to an Ogre skeleton
	bool ExSkeleton::createOgreBones(Ogre::SkeletonPtr pSkeleton)
	{
		// Doug Perkowski
		// 5/25/2010
		// To prevent a crash on content with more than  (256) bones, we need to check
		// to make sure there aren't more than OGRE_MAX_NUM_BONES bones.
		if(m_joints.size() > OGRE_MAX_NUM_BONES)
		{
      MessageBox(NULL, _T("Failure: Skeleton has more than OGRE_MAX_NUM_BONES.  No bones will be exported."), _T("EasyOgre Export Error"), MB_OK | MB_ICONWARNING);
			EasyOgreExporterLog("Failure: Skeleton has more than OGRE_MAX_NUM_BONES.  No bones will be exported.\n");
			return false;
		}
		
		// Create the bones
		for (size_t i = 0; i < m_joints.size(); i++)
		{
			joint* j = &m_joints[i];
			// Create a new bone
			Ogre::Bone* pBone = pSkeleton->createBone(m_joints[i].name.c_str(), m_joints[i].id);

			// Set bone position (relative to it's parent)
      pBone->setPosition(j->trans.x, j->trans.y, j->trans.z);

			// Set bone orientation (relative to it's parent)
      Ogre::Quaternion orient(j->rot.w, j->rot.x, j->rot.y, j->rot.z);
			pBone->setOrientation(orient);

			// Set bone scale (relative to it's parent
			pBone->setScale(j->scale.x, j->scale.y, j->scale.z);
		}

		// Create the hierarchy
		for (size_t i = 0; i < m_joints.size(); i++)
		{
			int parentIdx = m_joints[i].parentIndex;
			if (parentIdx >= 0)
			{
				// Get the parent joint
				Ogre::Bone* pParent = pSkeleton->getBone(m_joints[parentIdx].id);
				// Get current joint from skeleton
				Ogre::Bone* pBone = pSkeleton->getBone(m_joints[i].id);
				// Place current bone in the parent's child list
				pParent->addChild(pBone);
			}
		}
		return true;
	}

	// Write skeleton animations to an Ogre skeleton
	bool ExSkeleton::createOgreSkeletonAnimations(Ogre::SkeletonPtr pSkeleton)
	{
		// Read loaded skeleton animations
    // parse the list reversed for good anims order
		for (size_t i = 0; i < m_animations.size(); i++)
		{
			// Create a new animation
			Ogre::Animation* pAnimation = pSkeleton->createAnimation(m_animations[i].m_name.c_str(), m_animations[i].m_length);

      // Create tracks for current animation
			for (size_t j = 0; j < m_animations[i].m_tracks.size(); j++)
			{
				ExTrack* t = &m_animations[i].m_tracks[j];
				
        // Create a new track
				Ogre::NodeAnimationTrack* pTrack = pAnimation->createNodeTrack(j,	pSkeleton->getBone(t->m_bone.c_str()));

				// Create keyframes for current track
				for (size_t k = 0; k < t->m_skeletonKeyframes.size(); k++)
				{
					skeletonKeyframe* keyframe = &t->m_skeletonKeyframes[k];

					// Create a new keyframe
					Ogre::TransformKeyFrame* pKeyframe = pTrack->createNodeKeyFrame(keyframe->time);

					// Set translation
					pKeyframe->setTranslate(Ogre::Vector3(keyframe->trans.x, keyframe->trans.y ,keyframe->trans.z));

					// Set rotation
          pKeyframe->setRotation(Ogre::Quaternion(keyframe->rot.w, keyframe->rot.x, keyframe->rot.y, keyframe->rot.z));

					// Set scale
					pKeyframe->setScale(Ogre::Vector3(keyframe->scale.x, keyframe->scale.y, keyframe->scale.z));
				}
			}
		}
		return true;
	}

};	//end namespace
