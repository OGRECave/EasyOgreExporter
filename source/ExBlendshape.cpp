////////////////////////////////////////////////////////////////////////////////
// ExBlendshape.cpp
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

#include "ExBlendshape.h"
#include "EasyOgreExporterLog.h"
namespace EasyOgreExporter
{
	// Constructor
	ExBlendShape::ExBlendShape(MorphR3* pMorphR3, IGameNode* pGameNode, IGameMesh* pGameMesh, ParamList &params)
	{
		clear();
		m_pMorphR3 = pMorphR3;
		m_pGameMesh = pGameMesh;
    m_pGameNode = pGameNode;
    m_params = params;
	}

	// Destructor
	ExBlendShape::~ExBlendShape()
	{
		clear();
	}

	// Clear blend shape data
	void ExBlendShape::clear()
	{
		m_pMorphR3 = 0;
		m_pGameMesh = 0;
    m_pGameNode = 0;
		m_origWeights.clear();
		m_poseGroups.clear();
		poseGroup pg;
		pg.targetIndex = 0;
		m_poseGroups.insert(std::pair<int,poseGroup>(0, pg));
		m_target = T_MESH;
	}

	// Load blend shape poses
	/*bool ExBlendShape::loadPoses(std::vector<vertex> &vertices,long numVertices,long offset,long targetIndex)
	{
		if (m_params.useSharedGeom)
		{
			assert(targetIndex == 0);
			m_target = T_MESH;
		}
		else
		{
			assert(offset == 0);
			poseGroup new_pg;
			m_target = T_SUBMESH;
			new_pg.targetIndex = targetIndex;
			m_poseGroups.insert(std::pair<int,poseGroup>(targetIndex,new_pg));
		}
		poseGroup& pg = m_poseGroups.find(targetIndex)->second;

		if(m_pGameMesh && m_pMorphR3)
		{
      // Disable all skin Modifiers.
      std::vector<Modifier*> disabledSkinModifiers;
      IGameObject* pGameObject = m_pGameNode->GetIGameObject();
      if(pGameObject)
      {
        int numModifiers = pGameObject->GetNumModifiers();
        for(int i = 0; i < numModifiers; ++i)
        {
          IGameModifier* pGameModifier = pGameObject->GetIGameModifier(i);
          if(pGameModifier)
          {
            if(pGameModifier->IsSkin())
            {
              Modifier* pModifier = pGameModifier->GetMaxModifier();
              if(pModifier)
              {
                if(pModifier->IsEnabled())
                {
                  disabledSkinModifiers.push_back(pModifier);
                  pModifier->DisableMod();
                }
              }
            }
          }
        }
      }

			// Get the original mesh from the IGameNode.  Not using IGame here
			// since MorphR3 doesn't allow for it.  Also we don't know if our vertices
			// are in object or world space, so we'll just calculate diffs directly from 
			// the Max meshes and modify the coordinate system manually.  
			// Obtained method of getting mesh from 3D Studio Max SDK Training session by
			// David Lanier.
 			bool DeleteObjectWhenDone;
			const ObjectState& objectState = m_pGameNode->GetMaxNode()->EvalWorldState(GetCOREInterface()->GetTime());
			Object *origMeshObj = objectState.obj;
			if (!origMeshObj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
			{
				EasyOgreExporterLog("Could not access original mesh for morph target comparison.");
				return false;
			}

			// Calculate the DiffTM matrix.  This is the difference between the INode's world transform
			// which is used to calculate the morph verticies, and the IGameNode's world transform, which is used
			// to calculate the Ogre mesh's verticies.
			Matrix3 DiffTM = m_pGameNode->GetObjectTM(GetCOREInterface()->GetTime()).ExtractMatrix3();

			// The below code is not well tested as FaceFX needs content in the native coordinates.
			// I've seen the direction of the morph movement flipped on some content when in Y-up mode 
			// which sets the coordinate system to IGAME_OGL.
			// I can't get this to work on all the morph examples I have however.
			IGameConversionManager* pConversionManager = GetConversionManager();
			if(IGameConversionManager::IGAME_OGL == pConversionManager->GetCoordSystem())
			{			
				Matrix3 conv = Matrix3(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
				DiffTM = DiffTM * conv;
			}

			TriObject *origMeshTriObj = (TriObject *) origMeshObj->ConvertToType(GetCOREInterface()->GetTime(), Class_ID(TRIOBJ_CLASS_ID, 0));
			if (origMeshObj != origMeshTriObj) DeleteObjectWhenDone = true;
			Mesh& origMesh = origMeshTriObj->GetMesh();
			const int NumVerts = origMesh.getNumVerts();
 

			for(int i = 0; i < m_pMorphR3->chanBank.size() && i < MR3_NUM_CHANNELS; ++i)
			{
				if(m_pMorphR3->chanBank[i].mActive)
				{
					morphChannel* pMorphChannel = &m_pMorphR3->chanBank[i];	
					if(pMorphChannel)
					{
						pMorphChannel->rebuildChannel();

						std::string posename = pMorphChannel->mName;
						int numMorphVertices = pMorphChannel->mNumPoints;
						
						if(numMorphVertices != origMesh.getNumVerts())
						{
							MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Morph targets have failed to export becuase the morph vertex count did not match the base mesh.  Collapse the modifier stack prior to export, as smoothing is not supported with morph target export."), _T("Morph Target Export Failed."), MB_OK);
							return false;
						}
						else
						{
							EasyOgreExporterLog("Exporting Morph target: %s with %d vertices.\n", posename.c_str(), numMorphVertices);
							EasyOgreExporterLog("Mesh has %d vertices.\n", numVertices);
							EasyOgreExporterLog("%d total vertices.\n", vertices.size());
							assert(offset+numVertices <= vertices.size());
							// create a new pose
							pose p;
							p.poseTarget = m_target;
							p.index = targetIndex;
							p.blendShapeIndex = i;
							p.name = posename;
							p.pMChannel = pMorphChannel;

							size_t numPoints = pMorphChannel->mPoints.size();
							std::vector<Point3> vmPoints;
							vmPoints.reserve(numPoints);
							for(size_t k = 0; k < numPoints; ++k)
							{
								vmPoints.push_back(pMorphChannel->mPoints[k]);
							}

							Box3 morphBoundingBox;
							// calculate vertex offsets
							for (int k=0; k<numVertices; k++)
							{
								vertexOffset vo;
								assert ((offset+k)<vertices.size());

								vertex v = vertices[offset+k];
								assert(v.index < numMorphVertices);
								assert(v.index < origMesh.getNumVerts());

								Point3 meshVert = origMesh.getVert(v.index);
								Point3 morphVert = vmPoints[v.index];

								Point3 diff = morphVert - meshVert;

								// Transform our morph vertex movements by whatever
								// scaling/rotation is being done by IGame..
								Point3 ogreSpacediff = DiffTM.VectorTransform(diff);


								// Add this point to the bounding box
								morphBoundingBox += morphVert;

								vo.x = ogreSpacediff.x * m_params.lum;
								vo.y = ogreSpacediff.y * m_params.lum;
								vo.z = ogreSpacediff.z * m_params.lum;	

								vo.index = offset+k;
								if (fabs(vo.x) < PRECISION)
									vo.x = 0;
								if (fabs(vo.y) < PRECISION)
									vo.y = 0;
								if (fabs(vo.z) < PRECISION)
									vo.z = 0;
								if ((vo.x!=0) || (vo.y!=0) || (vo.z!=0))
									p.offsets.push_back(vo);
							}
							// add pose to pose list
							if (p.offsets.size() > 0)
							{
								pg.poses.push_back(p);
							}
							if (params.bsBB)
							{
								// update bounding boxes of loaded submeshes
								for (int j=0; j<params.loadedSubmeshes.size(); j++)
								{
									params.loadedSubmeshes[j]->m_boundingBox += morphBoundingBox;
								}
							}
						}

					}
				}
			}

      // Re-enable skin modifiers.
      for(int i = 0; i < disabledSkinModifiers.size(); ++i)
      {
          disabledSkinModifiers[i]->EnableMod();
      }

			if (DeleteObjectWhenDone)
				origMeshTriObj->DeleteMe();
		}
		return true;
	}
  */

	// Load a blend shape animation track
	ExTrack ExBlendShape::loadTrack(float start,float stop,float rate,int targetIndex, int startPoseId)
	{
		int i;
		std::string msg;
		std::vector<float> times;

		// Create a track for current clip
		ExTrack t;
		t.m_type = TT_POSE;
		t.m_target = m_target;
		t.m_index = targetIndex;
		t.m_vertexKeyframes.clear();
		// Calculate times from clip sample rate
		times.clear();
		if (rate <= 0)
		{
			EasyOgreExporterLog("invalid sample rate for the clip (must be >0), we skip it\n");
			return t;
		}
		float time;
		for (time=start; time<stop; time+=rate)
			times.push_back(time);
		times.push_back(stop);
		// Get animation length
		float length=0;
		if (times.size() >= 0)
			length = times[times.size()-1] - times[0];
		if (length < 0)
		{
			EasyOgreExporterLog("invalid time range for the clip, we skip it\n");
			return t;
		}
		// Evaluate animation curves at selected times
		for (i=0; i<times.size(); i++)
		{
			vertexKeyframe key;
			key.time = times[i] - times[0];
			key.poserefs.clear();

			poseGroup& pg = m_poseGroups.find(targetIndex)->second;
			for (int j=0; j<pg.poses.size(); j++)
			{
				pose& p = pg.poses[j];
				morphChannel* pChan = p.pMChannel;	
				if(pChan)
				{
					IParamBlock* paramBlock = pChan->cblock;
					float value;
					TimeValue curTime = times[i] * TIME_TICKSPERSEC;
					Interval junkInterval;
					paramBlock->GetValue(0, curTime, value, junkInterval);
					vertexPoseRef poseref;
					poseref.poseIndex = startPoseId + j;
					poseref.poseWeight = value / 100.0f;
					key.poserefs.push_back(poseref);
				}
			}
			t.addVertexKeyframe(key);
		}
		// Clip successfully loaded
		return t;
	}

	// Get blend shape deformer name
	std::string ExBlendShape::getName()
	{
		if(m_pGameMesh)
			return m_pGameNode->GetName();
		else
			return "";
	}
	// Get blend shape poses
	stdext::hash_map<int,poseGroup>& ExBlendShape::getPoseGroups()
	{
		return m_poseGroups;
	}

} // end namespace
