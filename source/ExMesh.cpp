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
#include "ExMaterial.h"
#include "EasyOgreExporterLog.h"
#include "ExTools.h"
#include "OgreProgressiveMeshGenerator.h"
#include "OgreDistanceLodStrategy.h"
#include "IFrameTagManager.h"

//TODO bounding on skeleton

namespace EasyOgreExporter
{
  ExMesh::ExMesh(ExOgreConverter* converter, IGameNode* pGameNode, IGameMesh* pGameMesh, const std::string& name)
  {
    m_converter = converter;
    m_params = converter->getParams();
    m_name = name;
    m_GameMesh = pGameMesh;
    m_GameNode = pGameNode;
    m_GameSkin = 0;
    m_Mesh = 0;
    m_pSkeleton = 0;
    m_pMorphR3 = 0;
    m_SphereRadius = 0;
    m_numTextureChannel = 0;
    numOfVertices = 0;

    haveVertexColor = (pGameMesh->GetNumberOfColorVerts() > 0) ? true : false;
    haveVertexAlpha = (pGameMesh->GetNumberOfAlphaVerts() > 0) ? true : false;
    haveVertexIllum = (pGameMesh->GetNumberOfIllumVerts() > 0) ? true : false;
    
    std::vector<Modifier*> lModifiers;
    INode* node = m_GameNode->GetMaxNode();

    //get skin or morph modifier
    getModifiers();
    
    //disable morpher modifier
    if(m_pMorphR3)
      static_cast<Modifier*>(m_pMorphR3)->DisableMod();

    //force all skinned object to bind pos
    converter->setAllSkinToBindPos();

    //get the mesh in the current state
    bool delTri = false;
    TriObject* triObj = getTriObjectFromNode(node, GetFirstFrame(), delTri);
    Mesh* mMesh = &triObj->GetMesh();
    if(mMesh)
    {
      numOfVertices = mMesh->getNumVerts();

      offsetTM = GetNodeOffsetMatrix(node, m_params.yUpAxis);

      if (m_params.exportSkeleton && m_GameSkin)
      {
        // create the skeleton if it hasn't been created.
        EasyOgreExporterLog("Creating skeleton ...\n");
        if (!m_pSkeleton)
        {
          m_pSkeleton = new ExSkeleton(m_GameNode, m_GameSkin, GetGlobalNodeMatrix(node, m_params.yUpAxis, GetFirstFrame()), m_name, m_converter);
          m_pSkeleton->getVertexBoneWeights(mMesh->getNumVerts());
        }
      }

      //update the mesh normals
      mMesh->buildNormals();
      prepareMesh(mMesh);

      //free the tree object
      if(delTri && triObj)
      {
        triObj->DeleteThis();
        triObj = 0;
        delTri = false;
      }
    }

    //enable the morph modifiers again
    if(m_pMorphR3)
      static_cast<Modifier*>(m_pMorphR3)->EnableMod();

    //force all skinned object to bind pos
    converter->restoreAllSkin();
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

  std::vector<ExFace> ExMesh::GetFacesByMaterialId(int matId)
  {
    std::vector<ExFace> faces;
    for (int i = 0; i < m_faces.size(); i++)
    {
      if (m_faces[i].iMaterialId == matId)
        faces.push_back(m_faces[i]);
    }

    return faces;
  }

  void ExMesh::prepareMesh(Mesh* mMesh)
  {
    INode* node = m_GameNode->GetMaxNode();
    int numFaces = mMesh->getNumFaces();
    int numVertices = numFaces * 3;
    UVVert* vAlpha = mMesh->mapSupport(-VDATA_ALPHA) ? mMesh->mapVerts(-VDATA_ALPHA) : 0;
    int numMapChannels = mMesh->getNumMaps();
    std::vector<int> matIds;    

    // use the 3dsMax channel numbers so we keep the correct channel for material
    m_numTextureChannel = numMapChannels -1;
    
    /*
    //count texture channels
    (mMesh->numTVerts > 0) ? 1 : 0;
    for (size_t chan = 2; chan < numMapChannels; chan++)
    {
      if(mMesh->mapSupport(chan))
        m_numTextureChannel++;
    }
    */
    EasyOgreExporterLog("Info: Number of map channel (UV) found in this mesh : %i\n", m_numTextureChannel);
    
    // prepare faces table
    m_faces.resize(numFaces);
    
    std::multimap<long int, int> hash_table; //For fast lookup of vertices
    
    for (int i = 0; i < numFaces; ++i)
    {
      Face face = mMesh->faces[i];
      m_faces[i].vertices.resize(3);
      m_faces[i].iMaxId = i;
      m_faces[i].iMaterialId = mMesh->getFaceMtlIndex(i);
      matIds.push_back(m_faces[i].iMaterialId);
      
      for (size_t j = 0; j < 3; j++)
      {
        DWORD vIndex = face.getVert(j);
        ExVertex vertex(vIndex);

        Point3 pos = mMesh->getVert(vIndex);
        if(m_params.yUpAxis)
        {
          float py = pos.y;
          pos.y = pos.z;
          pos.z = -py;
        };
        pos = offsetTM.PointTransform(pos);

        //apply scale
        pos *= m_params.lum;

        Point3 normal = GetVertexNormals(mMesh, i, j, vIndex);
        if(m_params.yUpAxis)
        {
          float py = normal.y;
          normal.y = normal.z;
          normal.z = -py;
        };
        normal = offsetTM.VectorTransform(normal);
        
        Point3 color(0, 0, 0);
        Point4 fullColor(0, 0, 0, 1);
        if(haveVertexColor && mMesh->vcFace)
        {
          TVFace& vcface = mMesh->vcFace[i];
          const int VertexColorIndex = vcface.t[j];

          float alpha = 1.0;
          if(haveVertexAlpha && vAlpha)
            alpha = vAlpha[vIndex].x;
          
          color = mMesh->vertCol[VertexColorIndex];

          if((color.x == -1) && (color.x == -1) && (color.x == -1))
            color.x = color.y = color.z = 1;
          
          fullColor = Point4(color.x, color.y, color.z, alpha);
        }

        vertex.vPos = pos;
        vertex.vNorm = normal.Normalize();
        vertex.vColor = fullColor;

        //update bounding box
        updateBounds(pos);

        if(getSkeleton())
        {
          // save vertex bone weight
          vertex.lWeight = getSkeleton()->getWeightList(vIndex);
   
          // save joint ids
          vertex.lBoneIndex = getSkeleton()->getJointList(vIndex);
        }
        
        //default UV
        if(mMesh->numTVerts > 0)
        {
          //bad test
          //Point3 uv = (mMesh->numTVerts > vIndex) ? mMesh->tVerts[mMesh->tvFace[i].t[j]] : Point3(0.0f, 0.0f, 0.0f);
          Point3 uv = mMesh->tVerts[mMesh->tvFace[i].t[j]];
          uv.y = 1.0f - uv.y;
          vertex.lTexCoords.push_back(uv);
        }
        else if (numMapChannels > 1)
        {
          //add an empty uv to correspond to the max channel id
          vertex.lTexCoords.push_back(Point3(0.0f, 0.0f, 0.0f));
        }

        //extra textures channel
        for (size_t chan = 2; chan < numMapChannels; chan++)
        {
          Point3 uv = Point3(0.0f, 0.0f, 0.0f);
          if(mMesh->mapSupport(chan) && mMesh->mapFaces(chan) != 0)
          {
            TVFace tvFace = mMesh->mapFaces(chan)[i];
            uv = mMesh->mapVerts(chan)[tvFace.t[j]];
            uv.y = 1.0f - uv.y;
          }
          vertex.lTexCoords.push_back(uv);
        }

        //look if the vertex is already added
        int sameFound = -1;
        int m_vertices_size = m_vertices.size();

        const static float sensivity_multiplier = 1.0f/0.000001f; //sensivity taken from the ExVertex comparison code
		    long int key = (int)(vertex.vPos.x*sensivity_multiplier+0.5f) + (int)(vertex.vPos.y*sensivity_multiplier+0.5f) + (int)(vertex.vPos.z*sensivity_multiplier+0.5f); //Hashing function
        if (hash_table.count(key))
        {
            std::multimap<long int, int>::iterator itr;
            std::multimap<long int, int>::iterator lastElement;
            itr = hash_table.find(key);
            lastElement = hash_table.upper_bound(key);

            for ( ; itr != lastElement; ++itr)
            {
              if (m_vertices[itr->second] == vertex)
              {
                sameFound = itr->second;
                break;
              }
            }
        }

        //add vertex
        if(sameFound == -1)
        {
          hash_table.insert(std::multimap<long int, int>::value_type(key, m_vertices_size));
          m_vertices.push_back(vertex);
          m_faces[i].vertices[j] = m_vertices_size; //  instead of (m_vertices.size() -1)
        }
        else
        {
          m_faces[i].vertices[j] = sameFound;
        }
      }
    } // Loop faces.

    //sort submesh
    std::sort(matIds.begin(), matIds.end());
    matIds.erase(std::unique(matIds.begin(), matIds.end()), matIds.end());

    IGameMaterial* nodeMtl = m_GameNode->GetNodeMaterial();
    for(int matid = 0; matid < matIds.size(); matid++)
    {
      std::vector<ExFace> faces = GetFacesByMaterialId(matIds[matid]);
      
      //get the material by matId
      IGameMaterial* mat = GetSubMaterialByID(nodeMtl, matIds[matid]);

      ExMaterial* pMaterial = loadMaterial(mat);
      ExSubMesh submesh(matid, pMaterial);

      //construct a list of faces with the correct indices
      for (int fi = 0; fi < faces.size(); fi++)
      {
        //get global face
        ExFace face = faces[fi];
        
        ExFace sface;
        sface.iMaxId = face.iMaxId;
        sface.vertices.resize(3);

        for (size_t j = 0; j < 3; j++)
        {
          //get the vertex from the global vertices list
          ExVertex vertex(m_vertices[face.vertices[j]]);
          ExVertex svertex(vertex);
          
          submesh.m_vertices.push_back(svertex);
          sface.vertices[j] = submesh.m_vertices.size() -1;
        }

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
            m_GameSkin = static_cast<IGameSkin*>(pGameModifier);
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
      if(pObject != 0)
      {
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
              break;
            }
          }

          // continue with next derived object
          pObject = pDerivedObject->GetObjRef();
        }
      }
    }
  }

  // Get the subentities materials
  std::vector<ExMaterial*> ExMesh::getMaterials()
  {
    std::vector<ExMaterial*> lmat;
    for(int i = 0; i < m_subList.size(); i++)
    {
      lmat.push_back(m_subList[i].m_mat);
    }
    return lmat;
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
    for(int i=0; i < m_subList.size(); i++)
    {
      ExSubMesh subMesh = m_subList[i];
      //Generate submesh name
      std::string subName;
      std::stringstream strName;
      strName << subMesh.id;
      subName = strName.str();

      if (subMesh.m_faces.size() <= 0)
      {
        EasyOgreExporterLog("Warning: No faces found in submesh %d\n", subMesh.id);
        continue;
      }

      EasyOgreExporterLog("Info: create submesh : %s\n", subName.c_str());
      Ogre::SubMesh* pSubmesh = createOgreSubmesh(subMesh);
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
        Ogre::LodConfig lodConfig;
        lodConfig.levels.clear();
        lodConfig.mesh = pMesh;
		    lodConfig.strategy = Ogre::DistanceLodSphereStrategy::getSingletonPtr();

        // Percentage -> parametric
        //TODO from param
        Ogre::Real reduction = 0.1f;

        //TODO nb level in param
        //On distance
        float leveldist = m_SphereRadius * 2.0f;
        for(int nLevel = 0; nLevel < 4; nLevel++)
        {
          leveldist = (m_SphereRadius * 2.0f) * ((nLevel + 1) * (nLevel + 1));
          //leveldist = leveldist * (nLevel + 1);

          Ogre::LodLevel lodLevel;
          lodLevel.reductionMethod = Ogre::LodLevel::VRM_PROPORTIONAL;
          lodLevel.reductionValue = reduction * ((nLevel + 1) + (nLevel + 1));
          lodLevel.distance = leveldist;
          lodConfig.levels.push_back(lodLevel);

          EasyOgreExporterLog("Info: Generate mesh LOD Level : %f\n", leveldist);
        }
        
        Ogre::ProgressiveMeshGenerator pm;
        pm.generateLodLevels(lodConfig);

        //Ogre::ProgressiveMeshGenerator pm;
        //pm.generateAutoconfiguredLodLevels(pMesh);
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
    std::vector<int> animKeys = GetPointAnimationsKeysTime(m_GameNode, animRange, m_params.resampleAnims, m_params.resampleStep);
    
    bool delTri = false;
    TriObject* triObj = 0;
    Mesh* mesh = 0;

    std::vector<int> vertexIds;
    for(int v = 0; v < m_vertices.size(); v++)
    {
      vertexIds.push_back(m_vertices[v].iMaxId);
    }

    OptimizeMeshAnimation(node, animKeys, vertexIds);

    if(animKeys.size() > 0)
    {
      //look if any key change something before export
      bool isAnimated = false;
      for (int i = 0; i < animKeys.size() && !isAnimated; i++)
      {
        //get the mesh in the current state
        triObj = getTriObjectFromNode(node, animKeys[i], delTri);
        if(triObj)
          mesh = &triObj->GetMesh();

        if(mesh)
        {
          for(int v = 0; v < m_vertices.size() && !isAnimated; v++)
          {
            Point3 pos = mesh->getVert(vertexIds[v]);
            if(m_params.yUpAxis)
            {
              float py = pos.y;
              pos.y = pos.z;
              pos.z = -py;
            };
            pos = offsetTM.PointTransform(pos) * m_params.lum;
            if(m_vertices[v].vPos != pos)
              isAnimated = true;
          }
        }

        //free the tree object
        if(delTri && triObj)
        {
          triObj->DeleteThis();
          triObj = 0;
          delTri = false;
          mesh = 0;
        }
      }

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
          float ogreTime = static_cast<float>((kTime - animRange.Start()) / static_cast<float>(animRate)) / GetFrameRate();

          //get the mesh in the current state
          triObj = getTriObjectFromNode(node, animKeys[i], delTri);
          if(triObj)
            mesh = &triObj->GetMesh();
          
          if(mesh)
          {
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
              Point3 pos = mesh->getVert(m_vertices[v].iMaxId);
              if(m_params.yUpAxis)
              {
                float py = pos.y;
                pos.y = pos.z;
                pos.z = -py;
              };
              pos = offsetTM.PointTransform(pos) * m_params.lum;

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

          //free the tree object
          if(delTri && triObj)
          {
            triObj->DeleteThis();
            triObj = 0;
            delTri = false;
            mesh = 0;
          }
        }
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
            float ogreTime = static_cast<float>((kTime - animRange.Start()) / static_cast<float>(animRate)) / GetFrameRate();
            
            //get the mesh in the current state
            triObj = getTriObjectFromNode(node, animKeys[i], delTri);
            if(triObj)
              mesh = &triObj->GetMesh();
            
            if(mesh)
            {
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
                Point3 pos = mesh->getVert(lvert[v].iMaxId);
                if(m_params.yUpAxis)
                {
                  float py = pos.y;
                  pos.y = pos.z;
                  pos.z = -py;
                };
                pos = offsetTM.PointTransform(pos) * m_params.lum;
                
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

            //free the tree object
            if(delTri && triObj)
            {
              triObj->DeleteThis();
              triObj = 0;
              delTri = false;
              mesh = 0;
            }
          }
        }
      }
      animKeys.clear();
      return true;
    }
    return false;
  }

  void ExMesh::createMorphAnimations()
  {
    INode* node = m_GameNode->GetMaxNode();

    //Vertex animations
    if(!GetVertexAnimState(node))
      return;

    EasyOgreExporterLog("Loading vertex animations...\n");
   
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

        #ifdef UNICODE
          EasyOgreExporterLog("Info : mixer track found %ls\n", group->GetName());
        #else
          EasyOgreExporterLog("Info : mixer track found %s\n", group->GetName());
        #endif

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
            IBezFloatKey nkey;
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
    bool poseError = false;
    INode* node = m_GameNode->GetMaxNode();

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

    for(int i = 0; i < validChan.size(); i++)
    {
      morphChannel* pMorphChannel = validChan[i];
      pMorphChannel->rebuildChannel();

#ifdef UNICODE
      std::wstring posename_w = pMorphChannel->mName;
      std::string posename;
      posename.assign(posename_w.begin(), posename_w.end());
#else
      std::string posename = pMorphChannel->mName;
#endif
      int numMorphVertices = pMorphChannel->mNumPoints;
      //poses can have spaces before or after the name
      trim(posename);
            
      if(numMorphVertices != numOfVertices)
      {
        if (!poseError)
          MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Morph targets have failed to export because the morph vertex count did not match the base mesh.  Collapse the modifier stack prior to export, as smoothing is not supported with morph target export."), _T("Morph Target Export Failed."), MB_OK);
        
        poseError = true;
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
            pos = offsetTM.PointTransform(pos);

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
              pos = offsetTM.PointTransform(pos);

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
    if(m_pMorphR3->IsAnimated() && !poseError)
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

          #ifdef UNICODE
            EasyOgreExporterLog("Info : mixer track found %ls\n", group->GetName());
          #else
            EasyOgreExporterLog("Info : mixer track found %s\n", group->GetName());
          #endif

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
                clipFileName_s.assign(clipFileName_w.begin(), clipFileName_w.end());
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

  Ogre::SubMesh* ExMesh::createOgreSubmesh(ExSubMesh submesh)
  {
    int numVertices = submesh.m_vertices.size();

    // Create a new submesh
    Ogre::SubMesh* pSubmesh = m_Mesh->createSubMesh();
    pSubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;

    // Set material
    if(submesh.m_mat)
      pSubmesh->setMaterialName(submesh.m_mat->getName().c_str());
    
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
    int countTex = 0;
    for (size_t i = 0; i < m_numTextureChannel; i++)
    {
      decl->addElement(1, texSegmentSize, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, countTex);
      texSegmentSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
      countTex++;
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
                            *pCol = Ogre::VertexElement::convertColourValue(cv, Ogre::VertexElement::getBestColourVertexElementType());
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
    if (!pMaterial && !pGameMaterial)
    {
      // add the default material
      m_converter->getMaterialSet()->addMaterial();
      pMaterial = m_converter->getMaterialSet()->getMaterial(0);
    }
    else if (!pMaterial)
    {
      pMaterial = new ExMaterial(m_converter, pGameMaterial, m_params.resPrefix);
      m_converter->getMaterialSet()->addMaterial(pMaterial);
      pMaterial->load();
    }

    //loading complete
    return pMaterial;
  }

}; //end of namespace
