////////////////////////////////////////////////////////////////////////////////
// ExMesh.cpp
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

#include "ExMesh.h"
#include "EasyOgreExporterLog.h"
#include "ExTools.h"
#include "OgreProgressiveMesh.h"
#include "IFrameTagManager.h"

//TODO bounding on skleton

namespace EasyOgreExporter
{
  ExMesh::ExMesh(ExOgreConverter* converter, ParamList &params, IGameNode* pGameNode, IGameMesh* pGameMesh, const std::string& name)
  {
    m_converter = converter;
    m_params = params;
    m_name = name;
    m_GameMesh = pGameMesh;
    m_GameNode = pGameNode;
    m_Mesh = 0;
    m_pSkeleton = 0;
    m_pMorphR3 = 0;
    m_SphereRadius = 0;
    haveVertexColor = (pGameMesh->GetNumberOfColorVerts() > 0) ? true : false;
    haveVertexAlpha = (pGameMesh->GetNumberOfAlphaVerts() > 0) ? true : false;
    haveVertexIllum = (pGameMesh->GetNumberOfIllumVerts() > 0) ? true : false;

    getModifiers();
    buildVertices();
  }

  ExMesh::~ExMesh()
  {
    if(m_pSkeleton)
      delete m_pSkeleton;

    m_vertices.clear();
    m_faces.clear();
    m_subList.clear();
  }
  
  void ExMesh::updateBounds(Point3 pos)
  {
    Point3 minB = m_Bounding.Min();
    Point3 maxB = m_Bounding.Max();
    minB.x = std::min(minB.x, pos.x);
    minB.y = std::min(minB.y, pos.y);
    minB.z = std::min(minB.z, pos.z);
    maxB.x = std::max(maxB.x, pos.x);
    maxB.y = std::max(maxB.y, pos.y);
    maxB.z = std::max(maxB.z, pos.z);
    m_Bounding.pmin = minB;
    m_Bounding.pmax = maxB;

    m_SphereRadius = std::max(m_SphereRadius, pos.Length());
  }

  void ExMesh::buildVertices()
  {
    int numFaces = m_GameMesh->GetNumberOfFaces();
    Tab<int> mapChannels = m_GameMesh->GetActiveMapChannelNum();
    
    int numVertices = numFaces * 3;

    // prepare faces table
    m_faces.resize(numFaces);

    INode* node = m_GameNode->GetMaxNode();

    // Compute the pivot TM
		Matrix3 piv(1);
    piv.SetTrans(node->GetObjOffsetPos());
		PreRotateMatrix(piv, node->GetObjOffsetRot());
		ApplyScaling(piv, node->GetObjOffsetScale());
    
    Matrix3 transMT = TransformMatrix(piv, m_params.yUpAxis);
    
    for (int i = 0; i < numFaces; ++i)
    {
      FaceEx* face = m_GameMesh->GetFace(i);
      m_faces[i].vertices.resize(3);
      m_faces[i].iMaxId = face->meshFaceIndex;

      for (size_t j = 0; j < 3; j++)
      {
        ExVertex vertex(face->vert[j]);
        Point3 pos = transMT * m_GameMesh->GetVertex(face->vert[j], true);

        //apply scale
        pos *= m_params.lum;

        Point3 normal = m_GameMesh->GetNormal(face->norm[j], true);

        Point3 color(0, 0, 0);
        Point4 fullColor(0, 0, 0, 1);
        if(haveVertexColor && m_GameMesh->GetColorVertex(face->vert[j], color))
        {
          float alpha = 1.0f;
          if(!m_GameMesh->GetAlphaVertex(face->vert[j], alpha))
            alpha = 1.0f;
          
          if((color.x == -1) && (color.x == -1) && (color.x == -1))
            color.x = color.y = color.z = 0;
          
          fullColor = Point4(color.x, color.y, color.z, alpha);
        }

        vertex.vPos = pos;
        vertex.vNorm = normal;
        vertex.vColor = fullColor;

        //update bounding box
        updateBounds(pos);

        if(getSkeleton())
        {
          // save vertex bone weight
          vertex.lWeight = getSkeleton()->getWeightList(face->vert[j]);
   
          // save joint ids
          vertex.lBoneIndex = getSkeleton()->getJointList(face->vert[j]);
        }

        //TODO Tangent and Binormal
        for (size_t chan = 0; chan < mapChannels.Count(); chan++)
        {
          //skip vertex color, alpha and illum channels
          if(mapChannels[chan] > 0)
          {
            Point3 uv(0, 0, 0);
            if (m_GameMesh->GetMapVertex(mapChannels[chan], m_GameMesh->GetFaceTextureVertex(face->meshFaceIndex, j, mapChannels[chan]), uv))
            {
              uv.y = 1.0f - uv.y;
            }
            vertex.lTexCoords.push_back(uv);
          }
        }

        //look for a duplicated vertex
        int sameFound = -1;
        for(int vi = 0; vi < m_vertices.size() && (sameFound == -1); vi ++)
        {
          if (m_vertices[vi] == vertex)
          {
            sameFound = vi;
          }
        }

        //add vertex
        if(sameFound == -1)
        {
          m_vertices.push_back(vertex);
          m_faces[face->meshFaceIndex].vertices[j] = m_vertices.size() -1;
        }
        else
        {
          m_faces[face->meshFaceIndex].vertices[j] = sameFound;
        }
      }
    } // Loop faces.

    //sort submesh
    Tab<int> materialIDs = m_GameMesh->GetActiveMatIDs();
    for(int matid = 0; matid < materialIDs.Count(); matid++)
    {
      Tab<FaceEx*> faces = m_GameMesh->GetFacesFromMatID(materialIDs[matid]);
      ExSubMesh submesh(matid);

      //construct a list of faces with the correct indices
      for (int fi = 0; fi < faces.Count(); fi++)
      {
        //get global face
        ExFace face = m_faces[faces[fi]->meshFaceIndex];
        
        ExFace sface;
        sface.iMaxId = face.iMaxId;
        sface.vertices.resize(3);

        for (size_t j = 0; j < 3; j++)
        {
          //get the vertex from the global vertices list
          ExVertex vertex(m_vertices[face.vertices[j]]);
          ExVertex svertex(vertex);
          
          //look for a duplicated vertex
          /*
          int sameFound = -1;
          for(int vi = 0; vi < submesh.m_vertices.size() && (sameFound == -1); vi++)
          {
            if (submesh.m_vertices[vi] == vertex)
            {
              sameFound = vi;
            }
          }
          
          //add vertex
          if(sameFound == -1)
          {
            submesh.m_vertices.push_back(vertex);
            sface.vertices[j] = submesh.m_vertices.size() -1;
          }
          else
          {
            sface.vertices[j] = sameFound;
          }*/
          
          submesh.m_vertices.push_back(svertex);
          sface.vertices[j] = submesh.m_vertices.size() -1;
        }

        /*
        int sameFaceFound = -1;
        for(int sfi = 0; sfi < submesh.m_faces.size() && (sameFaceFound == -1); sfi++)
        {
          if (submesh.m_faces[sfi] == sface)
          {
            sameFaceFound = sfi;
          }
        }
        if(sameFaceFound == -1)
        {
          submesh.m_faces.push_back(sface);
        }*/
        submesh.m_faces.push_back(sface);
      }
      m_subList.push_back(submesh);
    }
  }

  ExSkeleton* ExMesh::getSkeleton()
  {
    return m_pSkeleton;
  }

  void ExMesh::getModifiers()
  {
    if(m_GameMesh)
    {
      int numModifiers = m_GameMesh->GetNumModifiers();
      for(int i = 0; i < numModifiers; ++i)
      {
        IGameModifier* pGameModifier = m_GameMesh->GetIGameModifier(i);
        if(pGameModifier)
        {
          if(pGameModifier->IsSkin())
          {
            IGameSkin* pGameSkin = static_cast<IGameSkin*>(pGameModifier);
            if(pGameSkin)
            {
              //replace the current mesh with the initial mesh before skin modifications
              m_GameMesh = pGameSkin->GetInitialPose();

              if (m_params.exportSkeleton && pGameSkin)
              {
                // create the skeleton if it hasn't been created.
                EasyOgreExporterLog("Creating skeleton ...\n");
                if (!m_pSkeleton)
                {
                  m_pSkeleton = new ExSkeleton(pGameSkin, m_name, m_params);
                  m_pSkeleton->getVertexBoneWeights(m_GameMesh);
                }
              }
            }
          }
          /*
          else if(pGameModifier->IsMorpher())
          {
            IGameMorpher* pGameMorpher = static_cast<IGameMorpher*>(pGameModifier);
            if(pGameMorpher)
            {
              Modifier* pModifier = pGameMorpher->GetMaxModifier();
              // Sanity check.
              if(MR3_CLASS_ID == pModifier->ClassID())
                m_pMorphR3 = static_cast<MorphR3*>(pModifier);
            }
          }
          */
        }
      }

      // get the object reference of the node
      Object* pObject = 0;
      pObject = m_GameNode->GetMaxNode()->GetObjectRef();
      if(pObject == 0)
        return;

      // loop through all derived objects
      while(pObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
      {
        IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObject);

        // loop through all modifiers
        int stackId;
        for(stackId = 0; stackId < pDerivedObject->NumModifiers(); stackId++)
        {
          // get the modifier
          Modifier* pModifier = pDerivedObject->GetModifier(stackId);

          // check if we found the morpher modifier
          if(pModifier->ClassID() == MR3_CLASS_ID)
          {
            m_pMorphR3 = static_cast<MorphR3*>(pModifier);
            return;
          }
        }

        // continue with next derived object
        pObject = pDerivedObject->GetObjRef();
      }
    }
  }

  // Write to a OGRE binary mesh
  bool ExMesh::writeOgreBinary()
  {
    int numVertices = m_vertices.size();

    // If no mesh have been exported, skip mesh creation
    if (numVertices <= 0)
    {
      EasyOgreExporterLog("Warning: No vertices found in this mesh\n");
      return false;
    }
    
    BOOL ignoreLOD = FALSE;
    IPropertyContainer* pc = m_GameMesh->GetIPropertyContainer();
    IGameProperty* pIgnoreLod = pc->QueryProperty(_T("noLOD"));
    if(pIgnoreLod)
      pIgnoreLod->GetPropertyValue(ignoreLOD);

    // Construct mesh
    Ogre::MeshPtr pMesh;
    try
    {
      pMesh = Ogre::MeshManager::getSingleton().createManual(m_name.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    }
    catch(Ogre::Exception &e)
    {
      EasyOgreExporterLog("Warning: Ogre exception :%s\n", e.what());
      return false;
    }

    // set Ogre::Mesh*
    m_Mesh = pMesh.getPointer();

    // Write shared geometry data
    if (m_params.useSharedGeom)
    {
      EasyOgreExporterLog("Info: Create Ogre shared geometry\n");
      createOgreSharedGeometry();
    }

    // rebuild vertices index, it must start on 0
    std::vector<std::vector<int>> facesIndex;
    int vertIndex = 0;

    //generate submesh
    EasyOgreExporterLog("Info: Create Ogre submeshs\n");
    Tab<int> materialIDs = m_GameMesh->GetActiveMatIDs();
    for(int i=0; i < m_subList.size(); i++)
    {
      //Generate submesh name
      std::string subName;
      std::stringstream strName;
      strName << materialIDs[i];
      subName = strName.str();

      Tab<FaceEx*> faces = m_GameMesh->GetFacesFromMatID(materialIDs[i]);
      if (faces.Count() <= 0)
      {
        EasyOgreExporterLog("Warning: No faces found in submesh %d\n", materialIDs[i]);
        continue;
      }

      ExMaterial* pMaterial = loadMaterial(m_GameMesh->GetMaterialFromFace(faces[0]));
      EasyOgreExporterLog("Info: create submesh : %s\n", subName.c_str());
      Ogre::SubMesh* pSubmesh = createOgreSubmesh(pMaterial, m_subList[i]);
      m_Mesh->nameSubMesh(subName, i);
    }

    //ignore instanciated mesh
    if(isFirstInstance(m_GameNode) == false)
    {
      EasyOgreExporterLog("Info: Ignore instanciated mesh\n");
      Ogre::MeshManager::getSingleton().remove(pMesh->getHandle());
      pMesh.setNull();
      m_Mesh = 0;
      return false;
    }

    // Create poses
    if (m_params.exportPoses && m_pMorphR3)
      createPoses();

    // Create poses
    if (m_params.exportVertAnims)
      createMorphAnimations();

    // Create a bounding box for the mesh
    EasyOgreExporterLog("Info: Create mesh bounding box\n");
    Ogre::AxisAlignedBox bbox = m_Mesh->getBounds();
    
    Point3 min1 = m_Bounding.Min();
    Point3 max1 = m_Bounding.Max();

    //reverse Z
    Ogre::Vector3 min2(min1.x, min1.y, min1.z);
    Ogre::Vector3 max2(max1.x, max1.y, max1.z);
    Ogre::AxisAlignedBox newbbox;
    newbbox.setExtents(min2, max2);
    bbox.merge(newbbox);

    // Define mesh bounds
    m_Mesh->_setBounds(bbox, false);
    m_Mesh->_setBoundingSphereRadius(m_SphereRadius);

    //free up some memory
    m_vertices.clear();
    m_faces.clear();
    m_subList.clear();

    //create LOD levels
    //don't do it on small meshs
    if((numVertices > 64) && m_params.generateLOD && !ignoreLOD)
    {
      EasyOgreExporterLog("Info: Generate mesh LOD\n");
      try
      {
        Ogre::ProgressiveMesh::VertexReductionQuota quota = Ogre::ProgressiveMesh::VRQ_PROPORTIONAL;

        // Percentage -> parametric
        //TODO from param
        Ogre::Real reduction = 0.15f;
        Ogre::Mesh::LodValueList valueList;

        //TODO nb level in param
        //On distance
        float leveldist = m_SphereRadius * 2.0f;
        for(int nLevel = 0; nLevel < 4; nLevel++)
        {
          leveldist = (m_SphereRadius * 2.0f) * ((nLevel + 1) * (nLevel + 1));
          //leveldist = leveldist * (nLevel + 1);

          valueList.push_back(leveldist);
          EasyOgreExporterLog("Info: Generate mesh LOD Level : %f\n", leveldist);
        }
        
        m_Mesh->setLodStrategy(Ogre::LodStrategyManager::getSingleton().getStrategy("Distance"));

        //On pixel, don't seems to work on mesh
        /*
        int leveldist = 256;
        for(int nLevel = 0; nLevel < 4; nLevel++)
        {
          leveldist = 256 / ((nLevel + 1) * (nLevel + 1));
          valueList.push_back(leveldist * leveldist);
          EasyOgreExporterLog("Info: Generate mesh LOD Level : %d\n", leveldist * leveldist);
        }
        
        m_Mesh->setLodStrategy(Ogre::LodStrategyManager::getSingleton().getStrategy("PixelCount"));
        */

        if(!(Ogre::ProgressiveMesh::generateLodLevels(m_Mesh, valueList, quota, reduction)))
        {
          m_Mesh->removeLodLevels();
          EasyOgreExporterLog("Error: Generating Mesh LOD levels\n");
        }
      }
      catch(Ogre::Exception &e)
      {
        EasyOgreExporterLog("Error: Generating Mesh LOD : %s\n", e.what());
      }
    }

    // see somewhere that it could avoid some memory leaks
    m_Mesh->load();

    // Build edges list
    if (m_params.buildEdges)
    {
      EasyOgreExporterLog("Info: Create mesh edge list\n");
      try
      {
        m_Mesh->buildEdgeList();
      }
      catch(Ogre::Exception e)
      {
        EasyOgreExporterLog("Warning: Can not create mesh edge list : %s\n", e.what());
      }
    }

    // Build tangents
    if (m_params.buildTangents)
    {
      EasyOgreExporterLog("Info: Create mesh tangents\n");
      Ogre::VertexElementSemantic targetSemantic = (m_params.tangentSemantic == TS_TANGENT) ? Ogre::VES_TANGENT : Ogre::VES_TEXTURE_COORDINATES;
      bool canBuild = true;
      unsigned short srcTex, destTex;
      try
      {
        canBuild = !m_Mesh->suggestTangentVectorBuildParams(targetSemantic, srcTex, destTex);
      }
      catch(Ogre::Exception e)
      {
        canBuild = false;
        EasyOgreExporterLog("Error: Creating mesh tangents failed : %s\n", e.what());
      }
      if (canBuild)
        m_Mesh->buildTangentVectors(targetSemantic, srcTex, destTex, m_params.tangentsSplitMirrored, m_params.tangentsSplitRotated, m_params.tangentsUseParity);
    }

    // Set skeleton link (if present)
    if (m_pSkeleton && m_params.exportSkeleton)
    {
      EasyOgreExporterLog("Info: Link Ogre skeleton\n");
      std::string filePath = optimizeFileName(m_name + ".skeleton");
        //makeOutputPath("", params.meshOutputDir, m_name, "skeleton";
      try
      {
        m_Mesh->setSkeletonName(filePath.c_str());
      }
      catch(Ogre::Exception &)
      {
        //ignore loading exception
      }
    }

    // Make sure animation types are up to date first
		m_Mesh->_determineAnimationTypes();

    // reorganize mesh buffers
    // Shared geometry
    if (m_Mesh->sharedVertexData)
    {
      EasyOgreExporterLog("Info: Optimize mesh\n");

      Ogre::VertexData* vdata = m_Mesh->getVertexDataByTrackHandle(0);
      Ogre::VertexDeclaration* newadcl = vdata->vertexDeclaration->getAutoOrganisedDeclaration(m_Mesh->hasSkeleton(), m_Mesh->hasVertexAnimation(), m_Mesh->getSharedVertexDataAnimationIncludesNormals());
      vdata->reorganiseBuffers(newadcl);

      // Automatic
      Ogre::VertexDeclaration* newDcl = m_Mesh->sharedVertexData->vertexDeclaration->getAutoOrganisedDeclaration(
      m_Mesh->hasSkeleton(), m_Mesh->hasVertexAnimation(), m_Mesh->getSharedVertexDataAnimationIncludesNormals());

      if (*newDcl != *(m_Mesh->sharedVertexData->vertexDeclaration))
      {
        // Usages don't matter here since we're onlly exporting
        Ogre::BufferUsageList bufferUsages;
        for (size_t u = 0; u <= newDcl->getMaxSource(); ++u)
          bufferUsages.push_back(Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

        m_Mesh->sharedVertexData->reorganiseBuffers(newDcl, bufferUsages);
      }
    }

    // Dedicated geometry
    Ogre::Mesh::SubMeshIterator smIt = m_Mesh->getSubMeshIterator();
    int subIdx = 0;
    while (smIt.hasMoreElements())
    {
      Ogre::SubMesh* sm = smIt.getNext();
      if (!sm->useSharedVertices)
      {
        const bool hasVertexAnim = sm->getVertexAnimationType() != Ogre::VAT_NONE;

        Ogre::VertexData* vdata = m_Mesh->getVertexDataByTrackHandle(subIdx + 1);
        Ogre::VertexDeclaration* newadcl = vdata->vertexDeclaration->getAutoOrganisedDeclaration(m_Mesh->hasSkeleton(), hasVertexAnim, sm->getVertexAnimationIncludesNormals());
        vdata->reorganiseBuffers(newadcl);

        // Automatic
        Ogre::VertexDeclaration* newDcl = sm->vertexData->vertexDeclaration->getAutoOrganisedDeclaration(
                m_Mesh->hasSkeleton(), hasVertexAnim, sm->getVertexAnimationIncludesNormals());
        if (*newDcl != *(sm->vertexData->vertexDeclaration))
        {
          // Usages don't matter here since we're onlly exporting
          Ogre::BufferUsageList bufferUsages;
          for (size_t u = 0; u <= newDcl->getMaxSource(); ++u)
            bufferUsages.push_back(Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

          sm->vertexData->reorganiseBuffers(newDcl, bufferUsages);
        }
      }
      subIdx++;
    }

    // Export the binary mesh
    Ogre::MeshSerializer serializer;
    std::string meshfile = makeOutputPath(m_params.outputDir, m_params.meshOutputDir, optimizeFileName(m_name), "mesh");

    EasyOgreExporterLog("Info: Write mesh file : %s\n", meshfile.c_str());
    try
    {
      serializer.exportMesh(m_Mesh, meshfile.c_str(), m_params.getOgreVersion());
    }
    catch(Ogre::Exception &e)
    {
      EasyOgreExporterLog("Error: Writing mesh file : %s : %s\n", meshfile.c_str(), e.what());
      pMesh.setNull();
      return false;
    }

    Ogre::MeshManager::getSingleton().remove(pMesh->getHandle());
    pMesh.setNull();
    m_Mesh = 0;
    return true;
  }

  bool ExMesh::exportMorphAnimation(Interval animRange, std::string name)
  {
    int animRate = GetTicksPerFrame();
    int animLenght = animRange.End() - animRange.Start();
    float ogreLenght = (static_cast<float>(animLenght) / static_cast<float>(animRate)) / GetFrameRate();

    // Compute the pivot TM
    INode* node = m_GameNode->GetMaxNode();
	  Matrix3 piv(1);
    piv.SetTrans(node->GetObjOffsetPos());
	  PreRotateMatrix(piv, node->GetObjOffsetRot());
	  ApplyScaling(piv, node->GetObjOffsetScale());

    Matrix3 transMT = TransformMatrix(piv, m_params.yUpAxis);
    std::vector<int> animKeys = GetPointAnimationsKeysTime(m_GameNode, animRange, m_params.resampleAnims);
    
    // Does a better way exist to get vertex position by time ?
    TimeValue initTime = GetCOREInterface()->GetTime();

    if(animKeys.size() > 0)
    {
      //look if any key change something before export
      bool isAnimated = false;
      for (int i = 0; i < animKeys.size() && !isAnimated; i++)
      {
        GetCOREInterface()->SetTime(animKeys[i], 0);
        for(int v = 0; v < m_vertices.size() && !isAnimated; v++)
        {
          Point3 pos = (transMT * m_GameMesh->GetVertex(m_vertices[v].iMaxId, true)) * m_params.lum;

          if(m_vertices[v].vPos != pos)
            isAnimated = true;
        }
      }
      GetCOREInterface()->SetTime(initTime);

      if(!isAnimated)
        return false;

      // Create a new animation for each clip
      Ogre::Animation* pAnimation = m_Mesh->createAnimation(name.c_str(), ogreLenght);

      if(m_params.useSharedGeom)
      {
        // Create a new track
        Ogre::VertexAnimationTrack* pTrack = pAnimation->createVertexTrack(0, m_Mesh->sharedVertexData, Ogre::VAT_MORPH);
      
        for (int i = 0; i < animKeys.size(); i++)
        {
          int kTime = animKeys[i];
          GetCOREInterface()->SetTime(kTime, 0);
          float ogreTime = static_cast<float>((kTime - animRange.Start()) / static_cast<float>(animRate)) / GetFrameRate();
          
          //add key frame
          Ogre::VertexMorphKeyFrame* pKeyframe = pTrack->createVertexMorphKeyFrame(ogreTime);

          // Create vertex buffer for current keyframe
          Ogre::HardwareVertexBufferSharedPtr pBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                                                          Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                          m_vertices.size(),
                                                          Ogre::HardwareBuffer::HBU_STATIC, true);
          float* pFloat = static_cast<float*>(pBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
          
          // Fill the vertex buffer with vertex positions
          for(int v = 0; v < m_vertices.size(); v++)
          {
            Point3 pos = (transMT * m_GameMesh->GetVertex(m_vertices[v].iMaxId, true)) * m_params.lum;
            
            //update bounding box
            updateBounds(pos);

            *pFloat++ = static_cast<float>(pos.x);
            *pFloat++ = static_cast<float>(pos.y);
            *pFloat++ = static_cast<float>(pos.z);
          }

          // Unlock vertex buffer
          pBuffer->unlock();

          // Set vertex buffer for current keyframe
          pKeyframe->setVertexBuffer(pBuffer);
        }
        GetCOREInterface()->SetTime(initTime);
      }
      else
      {
        // create a track for each submesh
        for(int sub = 0; sub < m_subList.size(); sub++)
        {
          std::vector<ExVertex> lvert = m_subList[sub].m_vertices;
          // Create a new track
          Ogre::VertexAnimationTrack* pTrack = pAnimation->createVertexTrack(sub+1, m_Mesh->getSubMesh(sub)->vertexData, Ogre::VAT_MORPH);

          for (int i = 0; i < animKeys.size(); i++)
          {
            int kTime = animKeys[i];
            GetCOREInterface()->SetTime(kTime, 0);
            float ogreTime = static_cast<float>((kTime - animRange.Start()) / static_cast<float>(animRate)) / GetFrameRate();
            
            //add key frame
            Ogre::VertexMorphKeyFrame* pKeyframe = pTrack->createVertexMorphKeyFrame(ogreTime);

            // Create vertex buffer for current keyframe
            Ogre::HardwareVertexBufferSharedPtr pBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                                                            Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                            lvert.size(),
                                                            Ogre::HardwareBuffer::HBU_STATIC, true);
            float* pFloat = static_cast<float*>(pBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
            
            // Fill the vertex buffer with vertex positions
            for(int v = 0; v < lvert.size(); v++)
            {
              Point3 pos = (transMT * m_GameMesh->GetVertex(lvert[v].iMaxId, true)) * m_params.lum;
              
              //update bounding box
              updateBounds(pos);

              *pFloat++ = pos.x;
              *pFloat++ = pos.y;
              *pFloat++ = pos.z;
            }

            // Unlock vertex buffer
            pBuffer->unlock();

            // Set vertex buffer for current keyframe
            pKeyframe->setVertexBuffer(pBuffer);
          }
        }
        GetCOREInterface()->SetTime(initTime);
      }
      animKeys.clear();
      return true;
    }
    return false;
  }

  void ExMesh::createMorphAnimations()
  {
    IGameControl* nodeControl = m_GameNode->GetIGameControl();

    //Vertex animations
    if(!nodeControl->IsAnimated(IGAME_POINT3))
      return;

    EasyOgreExporterLog("Loading vertex animations...\n");
   
    INode* node = m_GameNode->GetMaxNode();

    //try to get animations in motion mixer
    bool useDefault = true;
    IMixer8* mixer = TheMaxMixerManager.GetMaxMixer(node);
    if(mixer)
    {
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
              Interval animRange;
              int start;
              int stop;
              #ifdef PRE_MAX_2010
                std::string clipName = formatClipName(std::string(clip->GetFilename()), clipId);
              #else
              MaxSDK::AssetManagement::AssetUser &clipFile = const_cast<MaxSDK::AssetManagement::AssetUser&>(clip->GetFile());
#ifdef UNICODE
			  std::wstring clipFileName_w = clipFile.GetFileName();
			  std::string clipFileName_s;
			  clipFileName_s.assign(clipFileName_w.begin(),clipFileName_w.end());
                std::string clipName = formatClipName(clipFileName_s, clipId);
#else
                std::string clipName = formatClipName(std::string(clipFile.GetFileName()), clipId);
#endif
              #endif
              
              clipName.append("_morph");

              clip->GetGlobalBounds(&start, &stop);
              animRange.SetStart(start);
              animRange.SetEnd(stop);
              EasyOgreExporterLog("Info : mixer clip found %s from %i to %i\n", clipName.c_str(), start, stop);
              
              if(exportMorphAnimation(animRange, clipName))
                useDefault = false;
              
              clipId++;
            }
          }
          track->SetSolo(tMode);
        }
      }
    }

    if(useDefault)
    {
      Interval animRange = GetCOREInterface()->GetAnimRange();
      IFrameTagManager* frameTagMgr = static_cast<IFrameTagManager*>(GetCOREInterface(FRAMETAGMANAGER_INTERFACE));
      int cnt = frameTagMgr->GetTagCount();

      if(!cnt)
      {
        exportMorphAnimation(animRange, "default_morph");
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
#ifdef UNICODE
		  std::wstring name_w = frameTagMgr->GetNameByID(t);
		  std::string name_s;
		  name_s.assign(name_w.begin(),name_w.end());
          exportMorphAnimation(ianim, name_s);
#else
          exportMorphAnimation(ianim, std::string(frameTagMgr->GetNameByID(t)));
#endif
        }
      }
    }
  }

  bool ExMesh::exportPosesAnimation(Interval animRange, std::string name, std::vector<morphChannel*> validChan, std::vector<std::vector<int>> poseIndexList, bool bDefault)
  {
    int animRate = GetTicksPerFrame();
    int animLenght = animRange.End() - animRange.Start();
    float ogreLenght = (static_cast<float>(animLenght) / static_cast<float>(animRate)) / GetFrameRate();

    std::vector<int> animKeys;
    if(bDefault)
    {
      for(int pose = 0; pose < validChan.size(); pose++)
      {
        morphChannel* pMorphChannel = validChan[pose];

        //get keys for this pose
        IParamBlock* paramBlock = pMorphChannel->cblock;
        IKeyControl* ikeys = GetKeyControlInterface(paramBlock->GetController(0));
        if(ikeys)
        {
          for (int ik = 0; ik < ikeys->GetNumKeys(); ik++)
          {
            IKey nkey;
            ikeys->GetKey(ik, &nkey);

            if((nkey.time >= animRange.Start()) && (nkey.time <= animRange.End()))
            {
              animKeys.push_back(nkey.time);
            }
          }
        }
      }

      if(animKeys.size() > 0)
      {
        //sort and remove duplicated entries
        std::sort(animKeys.begin(), animKeys.end());
        animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
      }
    }
    else // use sampled keys
    {
     //add time steps
      for (int t = animRange.Start(); t < animRange.End(); t += animRate)
			  animKeys.push_back(t);

      //force the last key
		  animKeys.push_back(animRange.End());
      animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
    }
    
    if(animKeys.size() > 0)
    {
      //look if any key change something before export
      bool isAnimated = false;
      std::vector<float> prevWeights;
      for(int i = 0; i < validChan.size(); i++)
      {
        morphChannel* pMorphChannel = validChan[i];

        //get initial weight value for this pose
        float weight;
        Interval junkInterval;
        IParamBlock* paramBlock = pMorphChannel->cblock;
        paramBlock->GetValue(0, 0, weight, junkInterval);
        prevWeights.push_back(weight);
      }

      for (int i = 0; i < animKeys.size() && !isAnimated; i++)
      {
        for(int pose = 0; pose < validChan.size() && !isAnimated; pose++)
        {
          morphChannel* pMorphChannel = validChan[pose];

          //get weight value for this pose
          float weight;
          Interval junkInterval;
          IParamBlock* paramBlock = pMorphChannel->cblock;
          paramBlock->GetValue(0, animKeys[i], weight, junkInterval);
          if(prevWeights[pose] != weight)
            isAnimated = true;
        }
      }
      prevWeights.clear();

      if(!isAnimated)
        return false;

      // Create a new animation for each clip
      Ogre::Animation* pAnimation = m_Mesh->createAnimation(name.c_str(), ogreLenght);

      if(m_params.useSharedGeom)
      {
        // Create a new track
        Ogre::VertexAnimationTrack* pTrack = pAnimation->createVertexTrack(0, m_Mesh->sharedVertexData, Ogre::VAT_POSE);
      
        for (int i = 0; i < animKeys.size(); i++)
        {
          int kTime = animKeys[i];
          float ogreTime = static_cast<float>((kTime - animRange.Start()) / static_cast<float>(animRate)) / GetFrameRate();
          
          //add key frame
          Ogre::VertexPoseKeyFrame* pKeyframe = pTrack->createVertexPoseKeyFrame(ogreTime);

          for(int pose = 0; pose < validChan.size(); pose++)
          {
            morphChannel* pMorphChannel = validChan[pose];

            //get weight value for this pose
		        float weight;
		        Interval junkInterval;
            IParamBlock* paramBlock = pMorphChannel->cblock;
		        paramBlock->GetValue(0, kTime, weight, junkInterval);

            pKeyframe->addPoseReference(pose, weight / 100.0f);
          }
        }
      }
      else
      {
        // create a track for each submesh
        for(int sub = 0; sub < m_subList.size(); sub++)
        {
          // Create a new track
          Ogre::VertexAnimationTrack* pTrack = pAnimation->createVertexTrack(sub+1, m_Mesh->getSubMesh(sub)->vertexData, Ogre::VAT_POSE);
          
          for (int i = 0; i < animKeys.size(); i++)
          {
            int kTime = animKeys[i];
            float ogreTime = static_cast<float>((kTime - animRange.Start()) / static_cast<float>(animRate)) / GetFrameRate();
            
            //add key frame
            Ogre::VertexPoseKeyFrame* pKeyframe = pTrack->createVertexPoseKeyFrame(ogreTime);

            for(int pose = 0; pose < validChan.size(); pose++)
            {
              morphChannel* pMorphChannel = validChan[pose];

              //get weight value for this pose
		          float weight;
		          Interval junkInterval;
              IParamBlock* paramBlock = pMorphChannel->cblock;
		          paramBlock->GetValue(0, kTime, weight, junkInterval);

              pKeyframe->addPoseReference(poseIndexList[sub][pose], weight / 100.0f);
            }
          }
        }
      }
      animKeys.clear();
      return true;
    }
    return false;
  }

  void ExMesh::createPoses()
  {
    EasyOgreExporterLog("Loading poses and poses animations...\n");

    // Disable all skin Modifiers.
    std::vector<Modifier*> disabledSkinModifiers;
    IGameObject* pGameObject = m_GameNode->GetIGameObject();
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

    INode* node = m_GameNode->GetMaxNode();

    // Compute the pivot TM
		Matrix3 piv(1);
    piv.SetTrans(node->GetObjOffsetPos());
		PreRotateMatrix(piv, node->GetObjOffsetRot());
		ApplyScaling(piv, node->GetObjOffsetScale());
    
    std::vector<morphChannel*> validChan;
    for(int i = 0; i < m_pMorphR3->chanBank.size() && i < MR3_NUM_CHANNELS; ++i)
	  {
      morphChannel &pMorphChannel = m_pMorphR3->chanBank[i];
      if(pMorphChannel.mActive)
        validChan.push_back(&pMorphChannel);
    }

    //index for pose animations
    std::vector<std::vector<int>> poseIndexList;
    poseIndexList.resize(m_subList.size());
    int poseIndex = 0;

    Matrix3 transMT = TransformMatrix(piv, m_params.yUpAxis);
    for(int i = 0; i < validChan.size(); i++)
	  {
      morphChannel* pMorphChannel = validChan[i];
		  pMorphChannel->rebuildChannel();

#ifdef UNICODE
		  std::wstring posename_w = pMorphChannel->mName;
		  std::string posename;
		  posename.assign(posename_w.begin(),posename_w .end());
#else
		  std::string posename = pMorphChannel->mName;
#endif
		  int numMorphVertices = pMorphChannel->mNumPoints;
      //poses can have spaces before or after the name
      trim(posename);
			
      if(numMorphVertices != (m_GameMesh->GetNumberOfVerts()))
		  {
			  MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Morph targets have failed to export because the morph vertex count did not match the base mesh.  Collapse the modifier stack prior to export, as smoothing is not supported with morph target export."), _T("Morph Target Export Failed."), MB_OK);
		    break;
      }
		  else
		  {
			  EasyOgreExporterLog("Exporting Morph target: %s with %d vertices.\n", posename.c_str(), numMorphVertices);

			  size_t numPoints = pMorphChannel->mPoints.size();
			  std::vector<Point3> vmPoints;
			  vmPoints.reserve(numPoints);
			  for(size_t k = 0; k < numPoints; ++k)
			  {
				  vmPoints.push_back(pMorphChannel->mPoints[k]);
			  }

        if(m_params.useSharedGeom)
        {
          // Create a new pose for the ogre mesh
          Ogre::Pose* pPose = m_Mesh->createPose(0, posename.c_str());
          // Set the pose attributes
          for (int k = 0; k < m_vertices.size(); k++)
          {
            ExVertex vertex = m_vertices[k];
            Point3 pos = vmPoints[vertex.iMaxId];
            if(m_params.yUpAxis)
            {
              float vy = pos.y;
              pos.y = pos.z;
              pos.z = -vy;
            }
            pos = transMT * pos;

            // apply scale
            pos *= m_params.lum;
            
            //update bounding box
            updateBounds(pos);

            // diff
            pos -= vertex.vPos;
            pPose->addVertex(k, Ogre::Vector3(pos.x, pos.y, pos.z));
          }

          poseIndex ++;
        }
        else
        {
          for(int sub = 0; sub < m_subList.size(); sub++)
          {
            std::vector<ExVertex> verticesList = m_subList[sub].m_vertices;
            poseIndexList[sub].push_back(poseIndex);

            // Create a new pose for the ogre submesh
            Ogre::Pose* pPose = m_Mesh->createPose(sub + 1, posename.c_str());
            // Set the pose attributes
            for (int k = 0; k < verticesList.size(); k++)
            {
              ExVertex vertex = verticesList[k];
              Point3 pos = vmPoints[vertex.iMaxId];
              if(m_params.yUpAxis)
              {
                float vy = pos.y;
                pos.y = pos.z;
                pos.z = -vy;
              }
              pos = transMT * pos;

              // apply scale
              pos *= m_params.lum;

              //update bounding box
              updateBounds(pos);

              // diff
              pos -= vertex.vPos;
              pPose->addVertex(k, Ogre::Vector3(pos.x, pos.y, pos.z));
            }

            poseIndex ++;
          }
        }
      }
    }
    
    //Poses animations
    if(m_pMorphR3->IsAnimated())
    {
      //try to get animations in motion mixer
      bool useDefault = true;
      IMixer8* mixer = TheMaxMixerManager.GetMaxMixer(node);
      if(mixer)
      {
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
                Interval animRange;
                int start;
                int stop;
                #ifdef PRE_MAX_2010
                  std::string clipName = formatClipName(std::string(clip->GetFilename()), clipId);
                #else
                MaxSDK::AssetManagement::AssetUser &clipFile = const_cast<MaxSDK::AssetManagement::AssetUser&>(clip->GetFile());

#ifdef UNICODE
				std::wstring clipFileName_w = clipFile.GetFileName();
				std::string clipFileName_s;
				clipFileName_s.assign(clipFileName_w.begin(),clipFileName_w.end());
                std::string clipName = formatClipName(clipFileName_s, clipId);
#else
                std::string clipName = formatClipName(std::string(clipFile.GetFileName()), clipId);
#endif
                
                #endif
                
                clipName.append("_poses");

                clip->GetGlobalBounds(&start, &stop);
                animRange.SetStart(start);
                animRange.SetEnd(stop);
                EasyOgreExporterLog("Info : mixer clip found %s from %i to %i\n", clipName.c_str(), start, stop);
                
                if(exportPosesAnimation(animRange, clipName, validChan, poseIndexList, false))
                  useDefault = false;
                
                clipId++;
              }
            }
            track->SetSolo(tMode);
          }
        }
      }

      if(useDefault)
      {
        Interval animRange = GetCOREInterface()->GetAnimRange();
        IFrameTagManager* frameTagMgr = static_cast<IFrameTagManager*>(GetCOREInterface(FRAMETAGMANAGER_INTERFACE));
        int cnt = frameTagMgr->GetTagCount();

        if(!cnt)
        {
          exportPosesAnimation(animRange, "default_poses", validChan, poseIndexList, !m_params.resampleAnims);
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
#ifdef UNICODE
			std::wstring name_w = frameTagMgr->GetNameByID(t);
			std::string name_s;
			name_s.assign(name_w.begin(), name_w.end());
            exportPosesAnimation(ianim, name_s, validChan, poseIndexList, !m_params.resampleAnims);
#else
            exportPosesAnimation(ianim, std::string(frameTagMgr->GetNameByID(t)), validChan, poseIndexList, !m_params.resampleAnims);
#endif
          }
        }        
      }
    }

    // Re-enable skin modifiers.
    for(int i = 0; i < disabledSkinModifiers.size(); ++i)
    {
      disabledSkinModifiers[i]->EnableMod();
    }
  }

  bool ExMesh::createOgreSharedGeometry()
  {
    int facesCount = m_GameMesh->GetNumberOfFaces();
    if (facesCount <= 0)
    {
      EasyOgreExporterLog("Warning: No faces found in mesh\n");
      return false;
    }

    m_Mesh->sharedVertexData = new Ogre::VertexData();
    m_Mesh->sharedVertexData->vertexCount = m_vertices.size();

    buildOgreGeometry(m_Mesh->sharedVertexData, m_vertices);

    // Write vertex bone assignements list
    if (getSkeleton())
    {
      // Create a new vertex bone assignements list
      Ogre::Mesh::VertexBoneAssignmentList vbas;
      // Scan list of shared geometry vertices
      for (int i = 0; i < m_vertices.size(); i++)
      {
        ExVertex vertex = m_vertices[i];
        // Add all bone assignements for every vertex to the bone assignements list
        for (int j = 0; j < vertex.lWeight.size(); j++)
        {
          Ogre::VertexBoneAssignment vba;
          vba.vertexIndex = i;
          vba.boneIndex = vertex.lBoneIndex[j];
          vba.weight = vertex.lWeight[j];
          if (vba.weight > 0.0f)
            vbas.insert(Ogre::Mesh::VertexBoneAssignmentList::value_type(i, vba));
        }
      }
      // Rationalise the bone assignements list
      m_Mesh->_rationaliseBoneAssignments(m_Mesh->sharedVertexData->vertexCount, vbas);

      // Add bone assignements to the mesh
      for (Ogre::Mesh::VertexBoneAssignmentList::iterator bi = vbas.begin(); bi != vbas.end(); bi++)
        m_Mesh->addBoneAssignment(bi->second);

      m_Mesh->_compileBoneAssignments();
      m_Mesh->_updateCompiledBoneAssignments();
    }

    return true;
  }

  Ogre::SubMesh* ExMesh::createOgreSubmesh(ExMaterial* pMaterial, ExSubMesh submesh)
	{
    int numVertices = submesh.m_vertices.size();

    // Create a new submesh
		Ogre::SubMesh* pSubmesh = m_Mesh->createSubMesh();
    pSubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;

    // Set material
    if(pMaterial)
      pSubmesh->setMaterialName(pMaterial->getName().c_str());
    
    // Set use shared geometry flag
    pSubmesh->useSharedVertices = (m_Mesh->sharedVertexData) ? true : false;
		
    pSubmesh->vertexData = new Ogre::VertexData();
    pSubmesh->vertexData->vertexCount = numVertices;

    bool bUse32BitIndexes = ((numVertices > 65535) || (m_Mesh->sharedVertexData)) ? true : false;
		
		// Create a new index buffer
    pSubmesh->indexData->indexCount = numVertices;
    pSubmesh->indexData->indexBuffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
				                               bUse32BitIndexes ? Ogre::HardwareIndexBuffer::IT_32BIT : Ogre::HardwareIndexBuffer::IT_16BIT,
				                               pSubmesh->indexData->indexCount,
				                               Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		
    std::vector<std::vector<int>> facesIndex;
    facesIndex.resize(submesh.m_faces.size());
    if (!m_Mesh->sharedVertexData)
    {
      // rebuild vertices index, it must start on 0
      for (int i = 0; i < submesh.m_faces.size(); i++)
      {
        facesIndex[i].resize(3);
        for (size_t j = 0; j < 3; j++)
        {
          facesIndex[i][j] = submesh.m_faces[i].vertices[j];
        }
      }
    }
    else
    {
      for (int i = 0; i < submesh.m_faces.size(); i++)
      {
        facesIndex[i].resize(3);
        for (size_t j = 0; j < 3; j++)
        {
          facesIndex[i][j] = m_faces[submesh.m_faces[i].iMaxId].vertices[j];
        }
      }
    }

    // Fill the index buffer with faces data
	  if (bUse32BitIndexes)
    {
		  Ogre::uint32* pIdx = static_cast<Ogre::uint32*>(pSubmesh->indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
		  for (int i = 0; i < facesIndex.size(); i++)
		  {
			  *pIdx++ = static_cast<Ogre::uint32>(facesIndex[i][0]);
			  *pIdx++ = static_cast<Ogre::uint32>(facesIndex[i][1]);
			  *pIdx++ = static_cast<Ogre::uint32>(facesIndex[i][2]);
		  }
		  pSubmesh->indexData->indexBuffer->unlock();
    }
    else
    {
      Ogre::uint16* pIdx = static_cast<Ogre::uint16*>(pSubmesh->indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
      for (int i = 0; i < facesIndex.size(); i++)
	    {
			  *pIdx++ = static_cast<Ogre::uint16>(facesIndex[i][0]);
			  *pIdx++ = static_cast<Ogre::uint16>(facesIndex[i][1]);
			  *pIdx++ = static_cast<Ogre::uint16>(facesIndex[i][2]);
		  }
		  pSubmesh->indexData->indexBuffer->unlock();
	  }
    facesIndex.clear();

    if(!m_Mesh->sharedVertexData)
    {
      //build geometry
      buildOgreGeometry(pSubmesh->vertexData, submesh.m_vertices);

      // Write vertex bone assignements list
			if (getSkeleton())
			{
				// Create a new vertex bone assignements list
				Ogre::SubMesh::VertexBoneAssignmentList vbas;

				// Scan list of geometry vertices
        for (int i = 0; i < submesh.m_vertices.size(); i++)
        {
          ExVertex vertex = submesh.m_vertices[i];
          // Add all bone assignements for every vertex to the bone assignements list
          for (int j = 0; j < vertex.lWeight.size(); j++)
          {
            Ogre::VertexBoneAssignment vba;
            vba.vertexIndex = i;
            vba.boneIndex = vertex.lBoneIndex[j];
            vba.weight = vertex.lWeight[j];
            if (vba.weight > 0.0f)
              vbas.insert(Ogre::SubMesh::VertexBoneAssignmentList::value_type(i, vba));
          }
        }

				// Rationalise the bone assignements list
				pSubmesh->parent->_rationaliseBoneAssignments(pSubmesh->vertexData->vertexCount, vbas);
				// Add bone assignements to the submesh
				for (Ogre::SubMesh::VertexBoneAssignmentList::iterator bi = vbas.begin(); bi != vbas.end(); bi++)
				{
					pSubmesh->addBoneAssignment(bi->second);
				}
				pSubmesh->_compileBoneAssignments();
			}
		}
		return pSubmesh;
	}

  void ExMesh::buildOgreGeometry(Ogre::VertexData* vdata, std::vector<ExVertex> verticesList)
  {
    // A VertexDeclaration declares the format of a set of vertex inputs,
    Ogre::VertexDeclaration* decl = vdata->vertexDeclaration;
    // A VertexBufferBinding records the state of all the vertex buffer bindings required to
    // provide a vertex declaration with the input data it needs for the vertex elements.
    Ogre::VertexBufferBinding* bind = vdata->vertexBufferBinding;

    size_t vBufSegmentSize = 0;
    size_t texSegmentSize = 0;

    // Create the vertex declaration (the format of vertices).
    // Add vertex position
    decl->addElement(0, vBufSegmentSize,Ogre::VET_FLOAT3,Ogre::VES_POSITION);
    vBufSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);

    // Add vertex normal
    if (m_params.exportVertNorm)
    {
      decl->addElement(0, vBufSegmentSize, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
      vBufSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    }

    // Add vertex colour
    if (m_params.exportVertCol && haveVertexColor)
    {
      decl->addElement(0, vBufSegmentSize, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
      vBufSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR);
    }

    // Add texture coordinates
    //use VET_FLOAT2 to make ogre able to generate tangent ^^
    Tab<int> mapChannels = m_GameMesh->GetActiveMapChannelNum();
    int countTex = 0;
    for (size_t i = 0; i < mapChannels.Count(); i++)
    {
      if(mapChannels[i] > 0)
      {
        decl->addElement(1, texSegmentSize, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, countTex);
        texSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
        countTex++;
      }
    }

    // Now create the vertex buffers.
    Ogre::HardwareVertexBufferSharedPtr vbuf = Ogre::HardwareBufferManager::getSingletonPtr()
      ->createVertexBuffer(vBufSegmentSize, vdata->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
    Ogre::HardwareVertexBufferSharedPtr texBuf = Ogre::HardwareBufferManager::getSingletonPtr()
      ->createVertexBuffer(texSegmentSize, vdata->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
    
    // Bind them.
    bind->setBinding(0, vbuf);
    bind->setBinding(1, texBuf);

    // Lock them. pVert & pTexVert are pointers to the start of the hardware buffers.
    unsigned char* pVert = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
    unsigned char* pTexVert = static_cast<unsigned char*>(texBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
    Ogre::ARGB* pCol;
    float* pFloat;

    // Get the element lists for the buffers.
    Ogre::VertexDeclaration::VertexElementList elems = decl->findElementsBySource(0);
    Ogre::VertexDeclaration::VertexElementList texElems = decl->findElementsBySource(1);

    // Buffers are set up, so iterate the vertices.
    int iTexCoord = 0;
    for (int i = 0; i < verticesList.size(); ++i)
    {
      ExVertex vertex = verticesList[i];

      Ogre::VertexDeclaration::VertexElementList::const_iterator elemItr, elemEnd;
      elemEnd = elems.end();
      for (elemItr = elems.begin(); elemItr != elemEnd; ++elemItr)
      {
         // VertexElement corresponds to a part of a vertex definition eg. position, normal
         const Ogre::VertexElement& elem = *elemItr;
         switch (elem.getSemantic())
         {
         case Ogre::VES_POSITION:
            {
              elem.baseVertexPointerToElement(pVert, &pFloat);
              *pFloat++ = vertex.vPos.x;
              *pFloat++ = vertex.vPos.y;
              *pFloat++ = vertex.vPos.z;
              //EasyOgreExporterLog("Info: vertex %d pos : %f %f %f\n", i, vertex.vPos.x, vertex.vPos.y, vertex.vPos.z);
            }
            break;
         case Ogre::VES_NORMAL:
            {
              elem.baseVertexPointerToElement(pVert, &pFloat);
              *pFloat++ = vertex.vNorm.x;
              *pFloat++ = vertex.vNorm.y;
              *pFloat++ = vertex.vNorm.z;
              //EasyOgreExporterLog("Info: vertex %d normal : %f %f %f\n", i, vertex.vNorm.x, vertex.vNorm.y, vertex.vNorm.z);
            }
            break;
          case Ogre::VES_DIFFUSE:
            {
              elem.baseVertexPointerToElement(pVert, &pCol);
              Ogre::ColourValue cv(vertex.vColor.x, vertex.vColor.y, vertex.vColor.z, vertex.vColor.w);
							*pCol++ = Ogre::VertexElement::convertColourValue(cv, Ogre::VertexElement::getBestColourVertexElementType());
              //EasyOgreExporterLog("Info: vertex %d color : %f %f %f %f\n", i, vertex.vColor.x, vertex.vColor.y, vertex.vColor.z, vertex.vColor.w);
            }
            break;
         default:
            break;
         } // Deal with an individual vertex element (position or normal).
      } // Loop positions and normals.

      iTexCoord = 0;
      elemEnd = texElems.end();
      for (elemItr = texElems.begin(); elemItr != elemEnd; ++elemItr)
      {
        // VertexElement corresponds to a part of a vertex definition eg. tex coord
        const Ogre::VertexElement& elem = *elemItr;
        switch (elem.getSemantic())
        {
        case Ogre::VES_TEXTURE_COORDINATES:
          {
            elem.baseVertexPointerToElement(pTexVert, &pFloat);
            *pFloat++ = vertex.lTexCoords[iTexCoord].x;
            *pFloat++ = vertex.lTexCoords[iTexCoord].y;
            //*pFloat++ = vertex.lTexCoords[iTexCoord].z;
            //EasyOgreExporterLog("Info: vertex %d tex coord %d : %f %f\n", i, iTexCoord, vertex.lTexCoords[iTexCoord].x, vertex.lTexCoords[iTexCoord].y);
            iTexCoord++;
          }
          break;
        default:
          break;
         } // Deal with an individual vertex element (tex coords).
      } // Loop tex coords.

      pVert += vbuf->getVertexSize();
      pTexVert += texBuf->getVertexSize();
    } // Loop vertices.
    vbuf->unlock();
    texBuf->unlock();
  }

  ExMaterial* ExMesh::loadMaterial(IGameMaterial* pGameMaterial)
	{
    ExMaterial* pMaterial = 0;

    //try to load the material if it has already been created
		pMaterial = m_converter->getMaterialSet()->getMaterial(pGameMaterial);

    //otherwise create the material
		if (!pMaterial)
		{
      pMaterial = new ExMaterial(pGameMaterial, m_params.resPrefix);
			m_converter->getMaterialSet()->addMaterial(pMaterial);
      pMaterial->load(m_params);
		}

		//loading complete
		return pMaterial;
	}

}; //end of namespace
