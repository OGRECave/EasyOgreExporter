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
    m_pBlendShape = 0;
    
    getModifiers();
    buildVertices();

    // Load vertex animations
    if (m_params.exportVertAnims)
      loadAnims();

    // Load blend shapes
    if (m_params.exportBlendShapes && m_pBlendShape)
      loadBlendShapes();
  }

  ExMesh::~ExMesh()
  {
    if(m_pSkeleton)
      delete m_pSkeleton;

    if(m_pBlendShape)
      delete m_pBlendShape;

    m_vertices.clear();
    m_vertexClips.clear();
		m_BSClips.clear();
  }
  
  void ExMesh::buildVertices()
  {
    int numFaces = m_GameMesh->GetNumberOfFaces();
    Tab<int> mapChannels = m_GameMesh->GetActiveMapChannelNum();

    int numVertices = numFaces * 3;

    // prepare vertex table
    /*m_vertices.resize(numVertices);
    for (size_t i = 0; i < numVertices; i++)
    {
      m_vertices[i].iMaxId = -1;
      m_vertices[i].iNext = -1;
    }*/

    // prepare faces table
    m_faces.resize(numFaces);

    for (size_t i = 0; i < numFaces; ++i)
    {
      FaceEx* face = m_GameMesh->GetFace(i);
      m_faces[i].vertices.resize(3);

      for (size_t j = 0; j < 3; j++)
      {
        ExVertex vertex(face->vert[j]);
        Point3 pos = m_GameMesh->GetVertex(face->vert[j], true);
        //apply scale
        pos *= m_params.lum;

        Point3 normal = m_GameMesh->GetNormal(face->norm[j], true);
        Point3 color = m_GameMesh->GetColorVertex(face->vert[j]);
        float alpha = m_GameMesh->GetAlphaVertex(face->vert[j]);
        Point4 fullColor(color.x, color.y, color.z, alpha);

        vertex.vPos = pos;
        vertex.vNorm = normal;
        vertex.vColor = fullColor;
        vertex.iNext = -1;

        if(getSkeleton())
        {
          // save vertex bone weight
          vertex.lWeight = getSkeleton()->getWeightList(face->vert[j]);
   
          // save joint ids
          vertex.lBoneIndex = getSkeleton()->getJointList(face->vert[j]);
        }

        vertex.lTexCoords.resize(mapChannels.Count());
        for (size_t chan = 0; chan < mapChannels.Count(); chan++)
        {
          Point3 uv;
          if (m_GameMesh->GetMapVertex(mapChannels[chan], m_GameMesh->GetFaceTextureVertex(face->meshFaceIndex, j, mapChannels[chan]), uv))
          {
            uv.y = 1.0f - uv.y;
          }
          else
          {
            uv.x = 0;
            uv.y = 0;
            uv.z = 0;
          }
          vertex.lTexCoords[chan] = uv;
        }

        //look for duplicated vertex postition
        /*bool diff = true;
        int lastFound = face->vert[j];
        for (int k = face->vert[j]; k != -1 && diff; k = m_vertices[k].iNext)
        {
          //compare with current
          if(m_vertices[k] == vertex)
            diff = false;

          lastFound = k;
        }

        if(diff)
        {
          m_vertices[face->vert[j]] = vertex;
          m_faces[face->meshFaceIndex].vertices[j] = face->vert[j];
        }
        else*/
        {
          //add other vertex with the same position
          m_vertices.push_back(vertex);
          //save the new position to the last duplicated vertex found
          //m_vertices[lastFound].iNext = m_vertices.size() -1;
          m_faces[face->meshFaceIndex].vertices[j] = m_vertices.size() -1;
        }
      }
    } // Loop vertices.
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
          else if(pGameModifier->IsMorpher())
          {
            IGameMorpher* pGameMorpher = static_cast<IGameMorpher*>(pGameModifier);
            if(pGameMorpher)
            {
              Modifier* pModifier = pGameMorpher->GetMaxModifier();

              // Sanity check.
              if(MR3_CLASS_ID == pModifier->ClassID())
              {
                MorphR3* pMorphR3 = static_cast<MorphR3*>(pModifier);
                if(!m_pBlendShape)
                  m_pBlendShape = new ExBlendShape(pMorphR3, m_GameNode, m_GameMesh, m_params);
              }
            }
          }
        }
      }
    }
  }

  void ExMesh::loadAnims()
  {
    EasyOgreExporterLog("Loading vertex animations...\n");
    
    // clear animations data
    m_vertexClips.clear();

    //vertex animations 
    Interval animRange = GetCOREInterface()->GetAnimRange();
    //TODO determine if there is mesh animation keys
    //load clip
    //loadClip(params.vertClipList[i].name, start, stop, rate);
  }

  void ExMesh::loadBlendShapes()
  {
    //TODO
  }

  // Write to a OGRE binary mesh
  bool ExMesh::writeOgreBinary()
  {
    // If no mesh have been exported, skip mesh creation
    if (m_GameMesh->GetNumberOfVerts() <= 0)
    {
      EasyOgreExporterLog("Warning: No vertices found in this mesh\n");
      return false;
    }
    
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
    for(int i=0; i < materialIDs.Count(); i++)
    {
      //Generate submesh name
      std::string subName;
      std::stringstream strName;
      strName << materialIDs[i];
      subName = strName.str();

      Ogre::SubMesh* pSubmesh = createOgreSubmesh(materialIDs[i]);
      m_Mesh->nameSubMesh(subName, i);
    }

    // Set skeleton link (if present)
    if (m_pSkeleton && m_params.exportSkeleton)
    {
      EasyOgreExporterLog("Info: Link Ogre skeleton\n");
      std::string filePath = m_name + ".skeleton";
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

    // Shared geometry
    if (m_Mesh->sharedVertexData)
    {
      EasyOgreExporterLog("Info: Optimize mesh\n");

      // Automatic
      Ogre::VertexDeclaration* newDcl = 
      m_Mesh->sharedVertexData->vertexDeclaration->getAutoOrganisedDeclaration(
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
    while (smIt.hasMoreElements())
    {
      Ogre::SubMesh* sm = smIt.getNext();
      if (!sm->useSharedVertices)
      {
        const bool hasVertexAnim = sm->getVertexAnimationType() != Ogre::VAT_NONE;

        // Automatic
        Ogre::VertexDeclaration* newDcl = 
            sm->vertexData->vertexDeclaration->getAutoOrganisedDeclaration(
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
    }

    // Create a bounding box for the mesh
    EasyOgreExporterLog("Info: Create mesh bounding box\n");
    Ogre::AxisAlignedBox bbox = m_Mesh->getBounds();
    
    Point3 min1 = m_Bounding.Min();
    Point3 max1 = m_Bounding.Max();

    min1 = min1 * m_params.lum;
    max1 = max1 * m_params.lum;

    //reverse Z
    Ogre::Vector3 min2(min1.x, min1.y, min1.z);
    Ogre::Vector3 max2(max1.x, max1.y, max1.z);
    Ogre::AxisAlignedBox newbbox;
    newbbox.setExtents(min2, max2);
    bbox.merge(newbbox);

    // Define mesh bounds
    m_Mesh->_setBounds(bbox, false);

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
      }
      if (canBuild)
        m_Mesh->buildTangentVectors(targetSemantic, srcTex, destTex, m_params.tangentsSplitMirrored, m_params.tangentsSplitRotated, m_params.tangentsUseParity);
    }

    // Export the binary mesh
    Ogre::MeshSerializer serializer;
    std::string meshfile = makeOutputPath(m_params.outputDir, m_params.meshOutputDir, m_name, "mesh");

    EasyOgreExporterLog("Info: Write mesh file : %s\n", meshfile.c_str());
    //TODO manage Ogre version
    try
    {
      serializer.exportMesh(m_Mesh, meshfile.c_str());
    }
    catch(Ogre::Exception &e)
    {
      EasyOgreExporterLog("Error: Writing mesh file : %s : %s\n", meshfile.c_str(), e.what());
      pMesh.setNull();
      return false;
    }

    pMesh.setNull();
    return true;
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
      for (size_t i = 0; i < m_vertices.size(); i++)
      {
        ExVertex vertex = m_vertices[i];
        // Add all bone assignements for every vertex to the bone assignements list
        for (size_t j = 0; j < vertex.lWeight.size(); j++)
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

  Ogre::SubMesh* ExMesh::createOgreSubmesh(int matId)
	{
    Tab<FaceEx*> faces = m_GameMesh->GetFacesFromMatID(matId);
    if (faces.Count() <= 0)
    {
      EasyOgreExporterLog("Warning: No faces found in submesh %d\n", matId);
      return 0;
    }

    int numVertices = faces.Count() * 3;
    Material* pMaterial = loadMaterial(m_GameMesh->GetMaterialFromFace(faces[0]));

    // Create a new submesh
		Ogre::SubMesh* pSubmesh = m_Mesh->createSubMesh();
    pSubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;

    // Set material
    if(pMaterial)
      pSubmesh->setMaterialName(pMaterial->name().c_str());
    
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
    facesIndex.resize(faces.Count());
    if (!m_Mesh->sharedVertexData)
    {
      // rebuild vertices index, it must start on 0
      int vertIndex = 0;
      for (size_t i = 0; i < faces.Count(); i++)
      {
        facesIndex[i].resize(3);
        for (size_t j = 0; j < 3; j++)
        {
          facesIndex[i][j] = vertIndex;
          vertIndex++;
        }
      }
    }
    else
    {
      for (size_t i = 0; i < faces.Count(); i++)
      {
        facesIndex[i].resize(3);
        for (size_t j = 0; j < 3; j++)
        {
          facesIndex[i][j] = m_faces[faces[i]->meshFaceIndex].vertices[j];
        }
      }
    }

    // Fill the index buffer with faces data
	  if (bUse32BitIndexes)
    {
		  Ogre::uint32* pIdx = static_cast<Ogre::uint32*>(pSubmesh->indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
		  for (size_t i = 0; i < facesIndex.size(); i++)
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
      for (size_t i = 0; i < facesIndex.size(); i++)
	    {
			  *pIdx++ = static_cast<Ogre::uint16>(facesIndex[i][0]);
			  *pIdx++ = static_cast<Ogre::uint16>(facesIndex[i][1]);
			  *pIdx++ = static_cast<Ogre::uint16>(facesIndex[i][2]);
		  }
		  pSubmesh->indexData->indexBuffer->unlock();
	  }

    if(!m_Mesh->sharedVertexData)
    {
      //construct a list of faces with the correct indices
      std::vector<ExVertex> verticesList;
      for (size_t i = 0; i < faces.Count(); i++)
      {
        ExFace face = m_faces[faces[i]->meshFaceIndex];
        for (size_t j = 0; j < 3; j++)
          verticesList.push_back(m_vertices[face.vertices[j]]);
      }

      //build geometry
      buildOgreGeometry(pSubmesh->vertexData, verticesList);

      // Write vertex bone assignements list
			if (getSkeleton())
			{
				// Create a new vertex bone assignements list
				Ogre::SubMesh::VertexBoneAssignmentList vbas;

				// Scan list of shared geometry vertices
        for (size_t i = 0; i < verticesList.size(); i++)
        {
          ExVertex vertex = verticesList[i];
          // Add all bone assignements for every vertex to the bone assignements list
          for (size_t j = 0; j < vertex.lWeight.size(); j++)
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
				pSubmesh->parent->_rationaliseBoneAssignments(pSubmesh->vertexData->vertexCount,vbas);
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

  //TODO vertives index list against faces, needed for shared geometry
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
    if (m_params.exportVertCol)
    {
      decl->addElement(0, vBufSegmentSize, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
      vBufSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR);
    }

    // Add texture coordinates
    Tab<int> mapChannels = m_GameMesh->GetActiveMapChannelNum();
    for (size_t i = 0; i < mapChannels.Count(); i++)
    { 
      decl->addElement(1, texSegmentSize, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES, i);
      texSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
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
    Ogre::RGBA* pRGBA;
    float* pFloat;

    // Get the element lists for the buffers.
    Ogre::VertexDeclaration::VertexElementList elems = decl->findElementsBySource(0);
    Ogre::VertexDeclaration::VertexElementList texElems = decl->findElementsBySource(1);

    // Buffers are set up, so iterate the vertices.
    int iTexCoord = 0;
    for (size_t i = 0; i < verticesList.size(); ++i)
    {
      ExVertex vertex = verticesList[i];

      //update bounding box
      Point3 pos = vertex.vPos;
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

      Ogre::VertexDeclaration::VertexElementList::const_iterator elemItr, elemEnd;
      elemEnd = elems.end();
      for (elemItr = elems.begin(); elemItr != elemEnd; ++elemItr)
      {
         // VertexElement corresponds to a part of a vertex definition eg. position, normal
         const Ogre::VertexElement& elem = *elemItr;
         switch (elem.getSemantic())
         {
         case Ogre::VES_POSITION:
            elem.baseVertexPointerToElement(pVert, &pFloat);
            *pFloat++ = vertex.vPos.x;
            *pFloat++ = vertex.vPos.y;
            *pFloat++ = vertex.vPos.z;
            break;
         case Ogre::VES_NORMAL:
            elem.baseVertexPointerToElement(pVert, &pFloat);
            *pFloat++ = vertex.vNorm.x;
            *pFloat++ = vertex.vNorm.y;
            *pFloat++ = vertex.vNorm.z;
            break;
          case Ogre::VES_DIFFUSE:
            {
              elem.baseVertexPointerToElement(pVert, &pRGBA);
              Ogre::ColourValue col(vertex.vColor.x, vertex.vColor.y, vertex.vColor.z, vertex.vColor.w);
              *pRGBA = Ogre::VertexElement::convertColourValue(col, Ogre::VertexElement::getBestColourVertexElementType());
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
            *pFloat++ = vertex.lTexCoords[iTexCoord].z;
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

  Material* ExMesh::loadMaterial(IGameMaterial* pGameMaterial)
	{
    Material* pMaterial = 0;

    //try to load the material if it has already been created
		pMaterial = m_converter->getMaterialSet()->getMaterial(pGameMaterial);

    //otherwise create the material
		if (!pMaterial)
		{
      pMaterial = new Material(pGameMaterial, m_params.matPrefix);
			m_converter->getMaterialSet()->addMaterial(pMaterial);
      pMaterial->load(m_params);
		}

		//loading complete
		return pMaterial;
	}

}; //end of namespace
