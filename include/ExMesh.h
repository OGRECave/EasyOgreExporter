////////////////////////////////////////////////////////////////////////////////
// ExMesh.h
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

#ifndef _ExMesh_H
#define _ExMesh_H

#include "ExPrerequisites.h"
#include "ExOgreConverter.h"
#include "ExSkeleton.h"
#include "paramList.h"

namespace EasyOgreExporter
{
  class ExFace
  {
  public:
		  //constructor
      ExFace()
      {
      };

      //destructor
		  ~ExFace()
      {
        vertices.clear();
      };

      bool operator==(ExFace const& b)
      {
        if(vertices.size() != b.vertices.size())
          return false;

        if(vertices.size() != 3)
          return false;

        return ((vertices[0] == b.vertices[0]) && (vertices[1] == b.vertices[1]) && (vertices[2] == b.vertices[2])) ||
               ((vertices[0] == b.vertices[0]) && (vertices[1] == b.vertices[2]) && (vertices[2] == b.vertices[1])) ||
               ((vertices[0] == b.vertices[1]) && (vertices[1] == b.vertices[2]) && (vertices[2] == b.vertices[0])) ||
               ((vertices[0] == b.vertices[1]) && (vertices[1] == b.vertices[0]) && (vertices[2] == b.vertices[2])) ||
               ((vertices[0] == b.vertices[2]) && (vertices[1] == b.vertices[1]) && (vertices[2] == b.vertices[0])) ||
               ((vertices[0] == b.vertices[2]) && (vertices[1] == b.vertices[0]) && (vertices[2] == b.vertices[1]));
      };

      bool operator!=(ExFace const& b)
      {
        if(vertices.size() != b.vertices.size())
          return true;

        if(vertices.size() != 3)
          return true;

        return ((vertices[0] != b.vertices[0]) || (vertices[1] != b.vertices[1]) || (vertices[2] != b.vertices[2])) &&
               ((vertices[0] != b.vertices[0]) || (vertices[1] != b.vertices[2]) || (vertices[2] != b.vertices[1])) &&
               ((vertices[0] != b.vertices[1]) || (vertices[1] != b.vertices[2]) || (vertices[2] != b.vertices[0])) &&
               ((vertices[0] != b.vertices[1]) || (vertices[1] != b.vertices[0]) || (vertices[2] != b.vertices[2])) &&
               ((vertices[0] != b.vertices[2]) || (vertices[1] != b.vertices[1]) || (vertices[2] != b.vertices[0])) &&
               ((vertices[0] != b.vertices[2]) || (vertices[1] != b.vertices[0]) || (vertices[2] != b.vertices[1]));
      };

      // vertice indices
      std::vector<int> vertices;

      int iMaxId;
  };

  class ExVertex
  {
  public:
		  //constructor
      ExVertex()
      {
        iMaxId = -1;
      };

      ExVertex(const ExVertex &vert)
      {
        iMaxId = vert.iMaxId;
        vPos = Point3(vert.vPos);
        vNorm = Point3(vert.vNorm);
        vColor = Point4(vert.vColor);
        lTexCoords = vert.lTexCoords;
        lTangent = vert.lTangent;
        lBinormal = vert.lBinormal;
        lWeight = vert.lWeight;
        lBoneIndex = vert.lBoneIndex;
      };

		  //constructor
      ExVertex(int id)
      {
        iMaxId = id;
      };
  	  
      //destructor
		  ~ExVertex()
      {
        lTexCoords.clear();
        lTangent.clear();
        lBinormal.clear();
        lWeight.clear();
        lBoneIndex.clear();
      }

      bool cmpPointList(std::vector<Point3> l1, std::vector<Point3> l2, float sensivity)
      {
        if (l1.size() != l2.size())
          return false;

        bool ret = true;
        int i = 0;
        while(i < l1.size() && ret)
        {
          Point3 uv1 = l1[i];
          Point3 uv2 = l2[i];
          if(!(uv1.Equals(uv2, sensivity)))
            ret = false;

          i++;
        }

        return ret;
      };

      bool operator==(ExVertex const& b)
      {
        float sensivity = 0.000001f;
        return (vPos.Equals(b.vPos, sensivity) && 
            vNorm.Equals(b.vNorm, sensivity) && 
            vColor.Equals(b.vColor, sensivity) &&
            /*(iMaxId == b.iMaxId) &&*/ cmpPointList(lTexCoords, b.lTexCoords, sensivity) && cmpPointList(lTangent, b.lTangent, sensivity) && cmpPointList(lBinormal, b.lBinormal, sensivity));
      };

      bool operator!=(ExVertex const& b)
      {
        float sensivity = 0.000001f;
        return (!vPos.Equals(b.vPos, sensivity) ||
            !vNorm.Equals(b.vNorm, sensivity) || 
            !vColor.Equals(b.vColor, sensivity) ||
            /*(iMaxId != b.iMaxId) ||*/ (!cmpPointList(lTexCoords, b.lTexCoords, sensivity)) || (!cmpPointList(lTangent, b.lTangent, sensivity)) || (!cmpPointList(lBinormal, b.lBinormal, sensivity)));
      };

      int iMaxId;

      // next same vertex index in vertices list with the same position
      Point3 vPos;
      Point3 vNorm;
      Point4 vColor;
      std::vector<Point3> lTexCoords;
      std::vector<Point3> lTangent;
      std::vector<Point3> lBinormal;
      std::vector<float> lWeight;
      std::vector<int> lBoneIndex;
  };

  class ExSubMesh
  {
  public:
    std::vector<ExVertex> m_vertices;
    std::vector<ExFace> m_faces;
    int id;
  protected:
  private:

  public:
		//constructor
		ExSubMesh(int idx)
    {
      id = idx;
    };
	  
    //destructor
		~ExSubMesh()
    {
      m_vertices.clear();
      m_faces.clear();
    };

  protected:
  private:
  };

	class ExMesh
	{
  public:
  protected:
    std::string m_name;
    ExOgreConverter* m_converter;
    ParamList m_params;
    IGameMesh* m_GameMesh;
    IGameNode* m_GameNode;
    Ogre::Mesh* m_Mesh;
    MorphR3* m_pMorphR3;
    Box3 m_Bounding;
    float m_SphereRadius;
    ExSkeleton* m_pSkeleton;

    //faces with new vertices index
    std::vector<ExFace> m_faces;
    //cleaned and sorted vertices
    std::vector<ExVertex> m_vertices;
    std::vector<ExSubMesh> m_subList;
  private:

	public:
		//constructor
		ExMesh(ExOgreConverter* converter, ParamList &params, IGameNode* pGameNode, IGameMesh* pGameMesh, const std::string& name = "");
	  
    //destructor
		~ExMesh();
	
		//write to a OGRE binary mesh
		bool writeOgreBinary();
    
    // get pointer to linked skeleton
    ExSkeleton* getSkeleton();

  protected:
    void buildVertices();
    Ogre::SubMesh* createOgreSubmesh(ExMaterial* pMaterial, ExSubMesh submesh);
    bool createOgreSharedGeometry();
    void buildOgreGeometry(Ogre::VertexData* vdata, std::vector<ExVertex> verticesList);
		ExMaterial* loadMaterial(IGameMaterial* pGameMaterial);
    void getModifiers();
    void createPoses();
    bool exportPosesAnimation(Interval animRange, std::string name, std::vector<morphChannel*> validChan, std::vector<std::vector<int>> poseIndexList, bool bDefault);
    bool exportMorphAnimation(Interval animRange, std::string name);
    void createMorphAnimations();
    void updateBounds(Point3);

  private:
	};

}; // end of namespace

#endif
