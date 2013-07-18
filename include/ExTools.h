////////////////////////////////////////////////////////////////////////////////
// ExScene.h
// Author     : Bastien Bourineau
// Start Date : Junary 11th, 2012
// Copyright  : Copyright (c) 2011 OpenSpace3D
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
#ifndef _EXTOOLS_H
#define _EXTOOLS_H


#include "ExPrerequisites.h"
#include "decomp.h"
#include "iskin.h"
#include "IMixer8.h"
#include "iInstanceMgr.h"
#include "MeshNormalSpec.h"
#include <iostream>

#ifdef PRE_MAX_2010
#include "IPathConfigMgr.h"
#else
#include "IFileResolutionManager.h"
#endif	//PRE_MAX_2010 

inline bool invalidChar(char c) 
{ 
  return !(c>=0 && ((c > 44 && c < 47) || (c > 47 && c < 58) || (c > 64 && c < 91) || (c > 96 && c < 123))); 
} 

inline void trim(std::string& str)
{
  size_t spos = str.find_first_not_of(" \f\n\r\t\v");
  size_t lpos = str.find_last_not_of(" \f\n\r\t\v");
  if((std::string::npos == spos) || (std::string::npos == lpos))
    str.clear();
  else
    str = str.substr(spos, lpos-spos+1);
}

// Helper function for getting the filename from a full path.
inline std::string StripToTopParent(const std::string& filepath)
{
	int ri = filepath.find_last_of('\\');
	int ri2 = filepath.find_last_of('/');
	if(ri2 > ri)
	{
		ri = ri2;
	}
	return filepath.substr(ri+1);
}

inline std::string FilePath(std::string file)
{
  int ri = file.find_last_of('\\');
	int ri2 = file.find_last_of('/');
	if(ri2 > ri)
	{
		ri = ri2;
	}

  if (ri == file.npos)
    return file;

	return file.substr(0, ri+1);
}

inline std::string FileWithoutExt(std::string file)
{
  if(file.empty())
    return file;

  std::string name = StripToTopParent(file);
	int ri = name.find_last_of('.');
  if (ri == name.npos)
    return name;

	return name.substr(0, ri);
}

inline std::string FileExt(std::string file)
{
  if(file.empty())
    return file;

	int ri = file.find_last_of('.');
  if (ri == file.npos)
    return file;

	return file.substr(ri);
}

inline std::string ToLowerCase(std::string str)
{
  if(str.empty())
    return str;

  char* cstr = (char*)str.c_str();
  for (int i = 0; i < str.length(); i++)
  {
    cstr[i] = tolower(cstr[i]);
  }
  return str = cstr;
}

// Helper function to replace special chars for file names
inline std::string optimizeFileName(const std::string& filename)
{
  std::string newFilename = filename;
  if (isdigit(newFilename.c_str()[0]))
    newFilename.insert(0, "_");

  std::replace_if(newFilename.begin(), newFilename.end(), invalidChar, '_');
	return newFilename;
}

// Helper function to replace special chars for resources
inline std::string optimizeResourceName(const std::string& filename)
{
  std::string newFilename = filename;
  if (isdigit(newFilename.c_str()[0]))
    newFilename.insert(0, "_");

  std::replace_if(newFilename.begin(), newFilename.end(), invalidChar, '_');
	return newFilename;
}

// Helper function to generate full filepath
inline std::string makeOutputPath(std::string common, std::string dir, std::string file, std::string ext)
{
  std::string filePath = "";
  filePath.append(common);

  if (!common.empty())
  if ((common.compare((common.length() -2), 1, "/") != 0) && (common.compare((common.length() -2), 1, "\\") != 0))
    filePath.append("\\");

  filePath.append(dir);

  if (!dir.empty())
  if ((dir.compare((dir.length() -2), 1, "/") != 0) && (dir.compare((dir.length() -2), 1, "\\") != 0))
    filePath.append("\\");

  filePath.append(file);

  if(!ext.empty())
  {
    filePath.append(".");
    filePath.append(ext);
  }

  return filePath;
}

inline std::string formatClipName(std::string fname, int id)
{
  std::string sid;
  std::stringstream strid;
  strid << id;
  sid = strid.str();

  size_t dotIdx = fname.rfind(".", fname.length() -1);
  size_t folderIndexForward = fname.rfind("/", fname.length() -1);
  size_t folderIndexBackward = fname.rfind("\\", fname.length() -1);
  size_t folderIndex;
  if(folderIndexForward == std::string::npos)
  {
    folderIndex = folderIndexBackward;
  }
  else if(folderIndexBackward == std::string::npos)
  {
    folderIndex = folderIndexForward;
  }
  else
  {
    folderIndex = folderIndexBackward > folderIndexForward ? folderIndexBackward : folderIndexForward;
  }
  
  std::string newName = fname.substr(folderIndex + 1, (dotIdx - (folderIndex + 1))) + "_" + sid;
  trim(newName);
  return newName;
}


inline int GetFirstFrame()
{
  return GetCOREInterface()->GetAnimRange().Start();
}

inline bool IsBone(INode *pNode)
{
  if(pNode == NULL)
    return false; 

  ObjectState os = pNode->EvalWorldState(GetFirstFrame()); 
  if (!os.obj)
    return false;

  // bone type
  if(os.obj->ClassID() == Class_ID(BONE_CLASS_ID, 0))
    return true;

  // bided
  Control* cont = pNode->GetTMController();   
  if(cont->ClassID() == BIPSLAVE_CONTROL_CLASS_ID ||       //others biped parts    
          cont->ClassID() == BIPBODY_CONTROL_CLASS_ID)     //biped root "Bip01"     
  {
    return true;
  }

  return false;   
}

inline bool IsBiped(INode *pNode)
{
  if(pNode == NULL)
    return false; 

  ObjectState os = pNode->EvalWorldState(GetFirstFrame()); 
  if (!os.obj)
    return false;

  // bided
  Control* cont = pNode->GetTMController();   
  if(cont->ClassID() == BIPSLAVE_CONTROL_CLASS_ID ||       //others biped parts    
          cont->ClassID() == BIPBODY_CONTROL_CLASS_ID ||   //biped root "Bip01"
          cont->ClassID() == SKELOBJ_CLASS_ID ||
          cont->ClassID() == BIPED_CLASS_ID)
  {
    return true;
  }

  return false;   
}

inline bool IsPossibleBone(INode *pNode)
{
  if(pNode == NULL)
    return false; 

  // bided
  Control* cont = pNode->GetTMController();
  if(!cont)
    return false;

  if(cont->ClassID() == FOOTPRINT_CLASS_ID)     //biped foot print    
    return false;

  return true;   
}

inline Matrix3 TransformMatrix(Matrix3 orig_cur_mat, bool yUp)
{
  Matrix3 YtoZ;
  Matrix3 ZtoY;
  Matrix3 mat = orig_cur_mat;

  GMatrix gmat;
  gmat.SetRow(0, Point4(1,  0, 0, 0));
  gmat.SetRow(1, Point4(0,  0, 1, 0));
  gmat.SetRow(2, Point4(0, -1, 0, 0));
  gmat.SetRow(3, Point4(0,  0, 0, 1));
  YtoZ = gmat.ExtractMatrix3();
  ZtoY = Inverse(YtoZ);

  if (yUp)
    mat = YtoZ * orig_cur_mat * ZtoY;

 return(mat);
}

inline Matrix3 UniformMatrix(Matrix3 orig_cur_mat, bool yUp)
{
  AffineParts   parts;
  Matrix3       mat;

  Matrix3 YtoZ;
  Matrix3 ZtoY;

  GMatrix gmat;
  gmat.SetRow(0, Point4(1,  0, 0, 0));
  gmat.SetRow(1, Point4(0,  0, 1, 0));
  gmat.SetRow(2, Point4(0, -1, 0, 0));
  gmat.SetRow(3, Point4(0,  0, 0, 1));
  YtoZ = gmat.ExtractMatrix3();
  ZtoY = Inverse(YtoZ);

///Remove  scaling  from orig_cur_mat
//1) Decompose original and get decomposition info
  decomp_affine(orig_cur_mat, &parts); 

//2) construct 3x3 rotation from quaternion parts.q
  parts.q.MakeMatrix(mat);

//3) construct position row from translation parts.t  
  mat.SetRow(3,  parts.t);

  if (yUp)
    mat = YtoZ * mat * ZtoY;

 return(mat);
}

inline Matrix3 GetLocalUniformMatrix(INode *node, Matrix3 offsetMat, bool yUp, int t)
{
  //Decompose each matrix
  Matrix3 cur_mat = UniformMatrix(node->GetNodeTM(t), yUp) * Inverse(offsetMat);

  if (node->GetParentNode()->IsRootNode())
    return cur_mat;

  Matrix3 par_mat = UniformMatrix(node->GetParentNode()->GetNodeTM(t), yUp) * Inverse(offsetMat);
  
  //then return relative matrix in coordinate space of parent
  return cur_mat * Inverse(par_mat);
}

inline Matrix3 GetLocalUniformMatrix(INode *node, bool yUp, int t)
{
  //Decompose each matrix
  Matrix3 cur_mat = UniformMatrix(node->GetNodeTM(t), yUp);
 
  if (node->GetParentNode()->IsRootNode())
    return cur_mat;

  Matrix3 par_mat = UniformMatrix(node->GetParentNode()->GetNodeTM(t), yUp);
  
  //then return relative matrix in coordinate space of parent
  return cur_mat * Inverse(par_mat);
}

inline Matrix3 GetLocalUniformMatrix(INode *node, INode *parent, Matrix3 offsetMat, bool yUp, int t)
{
  //Decompose each matrix
  Matrix3 cur_mat = UniformMatrix(node->GetNodeTM(t), yUp) * Inverse(offsetMat);

  Matrix3 par_mat = TransformMatrix(parent->GetNodeTM(t), yUp) * Inverse(offsetMat);
  
  //then return relative matrix in coordinate space of parent
  return cur_mat * Inverse(par_mat);
}

inline Matrix3 GetRelativeMatrix(Matrix3 mat1, Matrix3 mat2)
{
  return mat1 * Inverse(mat2);
}


//Simple node transform
inline Matrix3 GetNodeOffsetMatrix(INode* node, bool yUp)
{
  Matrix3 piv(1);
  piv.PreTranslate(node->GetObjOffsetPos());
	PreRotateMatrix(piv, node->GetObjOffsetRot());
	ApplyScaling(piv, node->GetObjOffsetScale());
  
  return TransformMatrix(piv, yUp);
}

inline Matrix3 GetGlobalNodeMatrix(INode *node, bool yUp = false, int t = 0)
{
  return TransformMatrix(node->GetNodeTM(t), yUp);
}

inline Matrix3 GetLocalNodeMatrix(INode *node, bool yUp = false, int t = 0)
{
  Matrix3 nodeTM = TransformMatrix(node->GetNodeTM(t), yUp);
  
  if (node->GetParentNode()->IsRootNode())
    return nodeTM;

  Matrix3 parentTM = TransformMatrix(node->GetParentNode()->GetNodeTM(t), yUp);
  return nodeTM * Inverse(parentTM);
}

inline Matrix3 GetLocalNodeMatrix(INode *node, Matrix3 offsetTM, bool yUp = false, int t = 0)
{
  Matrix3 nodeTM = TransformMatrix(node->GetNodeTM(t), yUp) * Inverse(offsetTM);
  
  if (node->GetParentNode()->IsRootNode())
    return nodeTM;

  Matrix3 parentTM = TransformMatrix(node->GetParentNode()->GetNodeTM(t), yUp) * Inverse(offsetTM);
  return nodeTM * Inverse(parentTM);
}


//Biped interface
inline IBipMaster* GetBipedMasterInterface(IGameSkin* skin)
{
  if(!skin)
    return 0;

  IBipMaster* bipMaster = 0;
  for (int i = 0; (i < skin->GetTotalBoneCount()) && (bipMaster == 0); i++)
  {
    INode* bone = skin->GetBone(i);
    Control* boneControl = bone->GetTMController();
    
    if (boneControl)
    {
      //Get the Biped master Interface from the controller
      bipMaster = (IBipMaster*) boneControl->GetInterface(I_BIPMASTER);
    }
  }
  return bipMaster;
}

inline void ReleaseBipedMasterInterface(IGameSkin* skin, IBipMaster* bipMaster)
{
  if(!skin || !bipMaster)
    return;

  Control* bipedControl = 0;
  for (int i = 0; (i < skin->GetTotalBoneCount()) && (bipedControl == 0) ; i++)
  {
    INode* bone = skin->GetBone(i);
    Control* boneControl = bone->GetTMController();
    if (boneControl->ClassID() == BIPBODY_CONTROL_CLASS_ID)
      bipedControl = boneControl;
  }

  if (bipedControl)
  {
    bipedControl->ReleaseInterface(I_BIPMASTER, bipMaster);
  }
}

inline void SetBipedToPreviousMode(IBipMaster* bipMaster, DWORD previousMode)
{
  if(!bipMaster)
    return;

  bipMaster->EndModes(BMODE_FIGURE, 0);
  bipMaster->BeginModes(previousMode, 0);
}

inline void SetBipedToBindPose(IBipMaster* bipMaster, DWORD& previousMode)
{
  if(!bipMaster)
    return;

  previousMode = bipMaster->GetActiveModes();
  bipMaster->EndModes(previousMode, 0);
  bipMaster->BeginModes(BMODE_FIGURE, 0);
}


// Units conversion
#define M2MM 0.001f
#define M2CM 0.01f
#define M2M  1.0f
#define M2KM 1000.0f
#define M2IN 0.0254f
#define M2FT 0.3048f
#define M2ML 1609.344f

inline float GetUnitValue(int unitType)
{
  float value = 1.0f;

  switch(unitType)
  {
    case UNITS_INCHES:
      value = M2IN;
    break;

    case UNITS_FEET:
      value = M2FT;
    break;

    case UNITS_MILES:
      value = M2ML;
    break;

    case UNITS_MILLIMETERS:
      value = M2MM;
    break;

    case UNITS_CENTIMETERS:
      value = M2CM;
    break;

    case UNITS_METERS:
      value = M2M;
    break;

    case UNITS_KILOMETERS:
      value = M2KM;
    break;
  }

  return value;
}

inline float ConvertToMeter(int unitType)
{
  return GetUnitValue(unitType);
}

inline void AddKeyTabToVector(IGameKeyTab tabkeys, Interval animRange, std::vector<int>* animKeys)
{
  for (int keyPos = 0; keyPos < tabkeys.Count(); keyPos++)
  {
    IGameKey nkey = tabkeys[keyPos];
    if((nkey.t >= animRange.Start()) && (nkey.t <= animRange.End()))
    {
      animKeys->push_back(nkey.t);
    }
  }
}

inline bool GetAnimationsPosKeysTime(IGameControl* pGameControl, Interval animRange, std::vector<int>* animKeys)
{
  if(pGameControl->IsAnimated(IGAME_POS))
  {
    IGameKeyTab tkeys;
    IGameControl::MaxControlType contType = pGameControl->GetControlType(IGAME_POS);

    if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetBezierKeys(tkeys, IGAME_POS))
	  {
      0;
	  }
	  else if(contType == IGameControl::IGAME_INDEPENDENT_POS)
	  {
      IGameKeyTab xkeys;
      if(pGameControl->GetBezierKeys(xkeys, IGAME_POS_X))
      {
        AddKeyTabToVector(xkeys, animRange, animKeys);
      }

      IGameKeyTab ykeys;
      if(pGameControl->GetBezierKeys(ykeys, IGAME_POS_Y))
      {
        AddKeyTabToVector(ykeys, animRange, animKeys);
      }

      IGameKeyTab zkeys;
      if(pGameControl->GetBezierKeys(zkeys, IGAME_POS_Z))
      {
        AddKeyTabToVector(zkeys, animRange, animKeys);
      }
    }
	  else if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetLinearKeys(tkeys, IGAME_POS))
    {
		  0;
    }
	  else if(contType == IGameControl::IGAME_LIST)
	  {
	    int subNum = pGameControl->GetNumOfListSubControls(IGAME_POS);
	    if(subNum)
	    {
		    for(int i= 0; i < subNum; i++)
	      {
		      IGameKeyTab SubKeys;
			    IGameControl* subCont = pGameControl->GetListSubControl(i, IGAME_POS);
          GetAnimationsPosKeysTime(subCont, animRange, animKeys);
        }
      }
      else
        return false;
    }
	  else
      return false;
    
    AddKeyTabToVector(tkeys, animRange, animKeys);

    return true;
  }

  return false;
}

inline bool GetAnimationsRotKeysTime(IGameControl* pGameControl, Interval animRange, std::vector<int>* animKeys)
{
  if(pGameControl->IsAnimated(IGAME_ROT))
  {
    IGameKeyTab tkeys;
    IGameControl::MaxControlType contType = pGameControl->GetControlType(IGAME_ROT);

    if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetTCBKeys(tkeys, IGAME_ROT))
	  {
      0;
	  }
    else if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetBezierKeys(tkeys, IGAME_ROT))
	  {
      0;
	  }
	  else if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetLinearKeys(tkeys, IGAME_ROT))
    {
		  0;
    }
    else if(contType == IGameControl::IGAME_EULER)
	  {
      IGameKeyTab xkeys;
      if(pGameControl->GetBezierKeys(xkeys, IGAME_EULER_X))
      {
        AddKeyTabToVector(xkeys, animRange, animKeys);
      }

      IGameKeyTab ykeys;
      if(pGameControl->GetBezierKeys(ykeys, IGAME_EULER_Y))
      {
        AddKeyTabToVector(ykeys, animRange, animKeys);
      }

      IGameKeyTab zkeys;
      if(pGameControl->GetBezierKeys(zkeys, IGAME_EULER_Z))
      {
        AddKeyTabToVector(zkeys, animRange, animKeys);
      }

      IGameKeyTab xlkeys;
      if(pGameControl->GetLinearKeys(xlkeys, IGAME_EULER_X))
      {
        AddKeyTabToVector(xlkeys, animRange, animKeys);
      }

      IGameKeyTab ylkeys;
      if(pGameControl->GetLinearKeys(ylkeys, IGAME_EULER_Y))
      {
        AddKeyTabToVector(ylkeys, animRange, animKeys);
      }

      IGameKeyTab zlkeys;
      if(pGameControl->GetLinearKeys(zlkeys, IGAME_EULER_Z))
      {
        AddKeyTabToVector(zlkeys, animRange, animKeys);
      }
	  }
	  else if(contType == IGameControl::IGAME_LIST)
	  {
	    int subNum = pGameControl->GetNumOfListSubControls(IGAME_ROT);
	    if(subNum)
	    {
		    for(int i= 0; i < subNum; i++)
	      {
		      IGameKeyTab SubKeys;
			    IGameControl* subCont = pGameControl->GetListSubControl(i, IGAME_ROT);
          GetAnimationsRotKeysTime(subCont, animRange, animKeys);
        }
      }
      else
        return false;
    }
	  else
      return false;
    
    AddKeyTabToVector(tkeys, animRange, animKeys);

    return true;
  }

  return false;
}

inline bool GetAnimationsScaleKeysTime(IGameControl* pGameControl, Interval animRange, std::vector<int>* animKeys)
{
  if(pGameControl->IsAnimated(IGAME_SCALE))
  {
    IGameKeyTab tkeys;
    IGameControl::MaxControlType contType = pGameControl->GetControlType(IGAME_SCALE);

    if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetBezierKeys(tkeys, IGAME_SCALE))
	  {
      0;
	  }
	  else if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetLinearKeys(tkeys, IGAME_SCALE))
    {
		  0;
    }
	  else if(contType == IGameControl::IGAME_LIST)
	  {
	    int subNum = pGameControl->GetNumOfListSubControls(IGAME_SCALE);
	    if(subNum)
	    {
		    for(int i= 0; i < subNum; i++)
	      {
		      IGameKeyTab SubKeys;
			    IGameControl* subCont = pGameControl->GetListSubControl(i, IGAME_SCALE);
          GetAnimationsScaleKeysTime(subCont, animRange, animKeys);
        }
      }
      else
        return false;
    }
	  else
      return false;
    
    AddKeyTabToVector(tkeys, animRange, animKeys);

    return true;
  }

  return false;
}

inline bool GetAnimationsPointKeysTime(IGameControl* pGameControl, Interval animRange, std::vector<int>* animKeys)
{
  if(pGameControl->IsAnimated(IGAME_POINT3))
  {
    IGameKeyTab tkeys;
    IGameControl::MaxControlType contType = pGameControl->GetControlType(IGAME_POINT3);

    if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetBezierKeys(tkeys, IGAME_POINT3))
	  {
      0;
	  }
	  else if(contType == IGameControl::IGAME_MAXSTD && pGameControl->GetLinearKeys(tkeys, IGAME_POINT3))
    {
		  0;
    }
	  else if(contType == IGameControl::IGAME_LIST)
	  {
	    int subNum = pGameControl->GetNumOfListSubControls(IGAME_POINT3);
	    if(subNum)
	    {
		    for(int i= 0; i < subNum; i++)
	      {
		      IGameKeyTab SubKeys;
			    IGameControl* subCont = pGameControl->GetListSubControl(i, IGAME_POINT3);
          GetAnimationsPointKeysTime(subCont, animRange, animKeys);
        }
      }
      else
        return false;
    }
	  else
      return false;
    
    AddKeyTabToVector(tkeys, animRange, animKeys);

    return true;
  }

  return false;
}

inline std::vector<int> GetAnimationsKeysTime(IGameNode* pGameNode, Interval animRange, bool resample)
{
  std::vector<int> animKeys;
  IGameControl* pGameControl = pGameNode->GetIGameControl();

  if(resample)
  {
    IGameKeyTab tkeys;
    if(pGameControl->GetFullSampledKeys(tkeys, 1, IGameControlType(IGAME_TM), true))
    {
      AddKeyTabToVector(tkeys, animRange, &animKeys);
      return animKeys;
    }
  }

  //add the first pos as a key
  animKeys.push_back(animRange.Start());

  if(pGameControl->IsAnimated(IGAME_POS))
  {
    if(!GetAnimationsPosKeysTime(pGameControl, animRange, &animKeys))
    {
      IGameKeyTab poskeys;
      if(pGameControl->GetFullSampledKeys(poskeys, 1, IGameControlType(IGAME_TM), true))
      {
        AddKeyTabToVector(poskeys, animRange, &animKeys);
      }
    }
  }

  if(pGameControl->IsAnimated(IGAME_ROT))
  {
    if(!GetAnimationsRotKeysTime(pGameControl, animRange, &animKeys))
    {
      IGameKeyTab rotkeys;
      if(pGameControl->GetFullSampledKeys(rotkeys, 1, IGameControlType(IGAME_TM), true))
      {
        AddKeyTabToVector(rotkeys, animRange, &animKeys);
      }
    }
  }

  if(pGameControl->IsAnimated(IGAME_SCALE))
  {
    if(!GetAnimationsScaleKeysTime(pGameControl, animRange, &animKeys))
    {
      IGameKeyTab scalekeys;
      if(pGameControl->GetFullSampledKeys(scalekeys, 1, IGameControlType(IGAME_TM), true))
      {
        AddKeyTabToVector(scalekeys, animRange, &animKeys);
      }
    }
  }

  //add the last pos as a key
  animKeys.push_back(animRange.End());

  //sort and remove duplicated entries
  if(animKeys.size() > 0)
  {
    std::sort(animKeys.begin(), animKeys.end());
    animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
  }

  return animKeys;
}

inline void OptimizeNodeAnimation(INode* maxNode, std::vector<int> &animKeys)
{
  if (animKeys.size() > 2)
  {
    Matrix3 prevKeyTM;
    Matrix3 nextKeyTM;
    Matrix3 keyTM;
    for (int i = 0; i < animKeys.size(); i++)
    {
      if (animKeys.size() > i + 2)
      {
        prevKeyTM = GetLocalNodeMatrix(maxNode, false, animKeys[i]);
        keyTM = GetLocalNodeMatrix(maxNode, false, animKeys[i + 1]);
        nextKeyTM = GetLocalNodeMatrix(maxNode, false, animKeys[i + 2]);

        if(keyTM.Equals(prevKeyTM) != 0 && keyTM.Equals(nextKeyTM) != 0)
        {
          //remove the key
          animKeys.erase(animKeys.begin() + (i + 1));
          i--;
        }
      }
    }
  }
}

inline TriObject* getTriObjectFromNode(INode *Node, TimeValue T, bool &Delete) 
{
	Delete = false;
	Object* Obj = Node->EvalWorldState(T).obj;

	if(Obj && Obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
	{
		TriObject* Tri = (TriObject*)Obj->ConvertToType(T, Class_ID(TRIOBJ_CLASS_ID, 0));
		if(Obj != Tri)
			Delete = true;

		return(Tri);
	}
	else 
	{
		return(NULL);
	}
}

inline bool IsMorphKeyEqual(const std::vector<int> vertices, Mesh firstMesh, Mesh secondMesh)
{
  bool equal = true;
  Point3 pos1;
  Point3 pos2;
  for(int v = 0; v < vertices.size() && equal; v++)
  {
    pos1 = firstMesh.getVert(vertices[v]);
    pos2 = secondMesh.getVert(vertices[v]);
    if(pos1.Equals(pos2) == 0)
      equal = false;
  }
  return equal;
}

inline void OptimizeMeshAnimation(INode* maxNode, std::vector<int> &animKeys, const std::vector<int> vertices)
{
  if (animKeys.size() > 2)
  {
    bool delPrevTri = false;
    bool delKeyTri = false;
    bool delNextTri = false;
    TriObject* prevTriObj = 0;
    TriObject* keyTriObj = 0;
    TriObject* nextTriObj = 0;
    Mesh prevMesh;
    Mesh keyMesh;
    Mesh nextMesh;

    for (int i = 0; i < animKeys.size(); i++)
    {
      if (animKeys.size() > i + 2)
      {
        //get the mesh in the current state
        prevTriObj = getTriObjectFromNode(maxNode, animKeys[i], delPrevTri);
        if(prevTriObj)
          prevMesh = prevTriObj->GetMesh();

        keyTriObj = getTriObjectFromNode(maxNode, animKeys[i + 1], delKeyTri);
        if(keyTriObj)
          keyMesh = keyTriObj->GetMesh();

        nextTriObj = getTriObjectFromNode(maxNode, animKeys[i + 2], delNextTri);
        if(nextTriObj)
          nextMesh = nextTriObj->GetMesh();

        if(IsMorphKeyEqual(vertices, keyMesh, prevMesh) && IsMorphKeyEqual(vertices, keyMesh, nextMesh))
        {
          //remove the key
          animKeys.erase(animKeys.begin() + (i + 1));
          i--;
        }

        //free the tree object
        if(delPrevTri && prevTriObj)
        {
          prevTriObj->DeleteThis();
          prevTriObj = 0;
          delPrevTri = false;
        }

        if(delKeyTri && keyTriObj)
        {
          keyTriObj->DeleteThis();
          keyTriObj = 0;
          delKeyTri = false;
        }

        if(delNextTri && nextTriObj)
        {
          nextTriObj->DeleteThis();
          nextTriObj = 0;
          delNextTri = false;
        }
      }
    }
  }
}

inline std::vector<int> GetPointAnimationsKeysTime(IGameNode* pGameNode, Interval animRange, bool resample)
{
  std::vector<int> animKeys;
  IGameControl* pGameControl = pGameNode->GetIGameControl();
  int animRate = GetTicksPerFrame();

  if(resample)
  {
    //add time steps
    for (int t = animRange.Start(); t < animRange.End(); t += animRate)
		  animKeys.push_back(t);

    //force the last key
	  animKeys.push_back(animRange.End());
    animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
    return animKeys;
  }

  //add the first pos as a key
  animKeys.push_back(animRange.Start());

  if(pGameControl->IsAnimated(IGAME_POINT3))
  {
    if(!GetAnimationsPointKeysTime(pGameControl, animRange, &animKeys))
    {
      //add time steps
      for (int t = animRange.Start(); t < animRange.End(); t += animRate)
        animKeys.push_back(t);
    }
  }

  //force the last key
  animKeys.push_back(animRange.End());
  animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());

  //sort and remove duplicated entries
  if(animKeys.size() > 0)
  {
    std::sort(animKeys.begin(), animKeys.end());
    animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
  }

  return animKeys;
}

inline std::string getFirstInstanceName(IGameNode* pGameNode)
{
  INodeTab nodeInstances;
  IInstanceMgr* instMgr = IInstanceMgr::GetInstanceMgr();
  
  //Get nodes instances
  instMgr->GetInstances(*(pGameNode->GetMaxNode()), nodeInstances);

  std::string nodeName;
#ifdef UNICODE
	std::wstring pNodeName_w = nodeInstances[nodeInstances.Count() - 1]->GetName();
	nodeName.assign(pNodeName_w.begin(), pNodeName_w.end());
#else
	nodeName = nodeInstances[nodeInstances.Count() - 1]->GetName();
#endif

  return nodeName;
}

inline bool isFirstInstance(IGameNode* pGameNode)
{
  INodeTab nodeInstances;
  IInstanceMgr* instMgr = IInstanceMgr::GetInstanceMgr();
  
  //Get nodes instances
  instMgr->GetInstances(*(pGameNode->GetMaxNode()), nodeInstances);

  if(nodeInstances[nodeInstances.Count() - 1] == pGameNode->GetMaxNode())
    return true;
  
  return false;
}

inline bool useSpaceWarpModifier(INode* node)
{
  return (node->GetProperty(PROPID_HAS_WSM) != 0);
}

/*
  Normal computing
*/

inline Point3 getVertexNormal(Mesh* mesh, int faceNo, RVertex* rv)
{
	Face* f = &mesh->faces[faceNo];
	DWORD smGroup = f->smGroup;
	int numNormals = 0;
	Point3 vertexNormal;
	
	// Is normal specified
	// SPCIFIED is not currently used, but may be used in future versions.
	if (rv->rFlags & SPECIFIED_NORMAL)
  {
		vertexNormal = rv->rn.getNormal();
	}
	// If normal is not specified it's only available if the face belongs
	// to a smoothing group
	else if ((numNormals = rv->rFlags & NORCT_MASK) != 0 && smGroup)
  {
		// If there is only one vertex is found in the rn member.
		if (numNormals == 1)
    {
			vertexNormal = rv->rn.getNormal();
		}
		else
    {
			// If two or more vertices are there you need to step through them
			// and find the vertex with the same smoothing group as the current face.
			// You will find multiple normals in the ern member.
			for (int i = 0; i < numNormals; i++)
      {
				if (rv->ern[i].getSmGroup() & smGroup)
        {
					vertexNormal = rv->ern[i].getNormal();
				}
			}
		}
	}
	else
  {
		// Get the normal from the Face if no smoothing groups are there
		vertexNormal = mesh->getFaceNormal(faceNo);
	}
	
	return vertexNormal;
}

// Compute the face and vertex normals
inline Point3 GetVertexNormals(Mesh *mesh, int fpos, int vpos, DWORD vId)
{
  MeshNormalSpec* nrm = mesh->GetSpecifiedNormals();
  
  if (nrm && nrm->GetNumNormals())
  {
    return nrm->GetNormal(fpos, vpos);
  }
  else
  {
    return getVertexNormal(mesh, fpos, mesh->getRVertPtr(vId));
  }
}


inline bool GetVertexAnimState(Animatable* anim)
{
  if (anim->IsAnimated() && (anim->SuperClassID() == CTRL_FLOAT_CLASS_ID || anim->SuperClassID() == CTRL_POINT3_CLASS_ID || anim->SuperClassID() == CTRL_POINT4_CLASS_ID))
    return true;

  for (int i = 0; i < anim->NumSubs(); i++)
  {
    Animatable* sanim = anim->SubAnim(i);
	  if (sanim && GetVertexAnimState(sanim))
		  return true;
  }

  return false;
}

inline IGameMaterial* GetSubMaterialByID(IGameMaterial* mat, int matId)
{
  if (mat && mat->IsMultiType() && mat->GetSubMaterialCount() > 0)
  {
    Mtl* maxmat = mat->GetMaxMaterial();
    if (maxmat->ClassID() == Class_ID(BAKE_SHELL_CLASS_ID, 0))
    {
      // get the 2nd material (the bake one)
      if (mat->GetSubMaterialCount() == 2)
      {
        mat = mat->GetSubMaterial(1);
        // if the material is empty we try the first one
        if(!mat)
          mat = mat->GetSubMaterial(0);
      }
      else
      {
        mat = mat->GetSubMaterial(0);
      }
    }
  }

  if (mat && mat->IsSubObjType())
  {
    IGameMaterial* nmat = 0;
    for(int i = 0; i < mat->GetSubMaterialCount(); i++)
    {
      if (mat->GetMaterialID(i) == matId)
      {
        nmat = mat->GetSubMaterial(i);
        break;
      }
    }
    
    return nmat;
  }
  else
  {
    return mat;
  }
}

inline bool GetMaxFilePath(MSTR file, std::string &fullPath)
{
  bool bFileExist = false;
	MaxSDK::Util::Path fileName(file);
  bFileExist = (DoesFileExist(file) == TRUE) ? true : false;

  if(!bFileExist)
	{
#ifdef PRE_MAX_2010
		if(IPathConfigMgr::GetPathConfigMgr()->SearchForXRefs(fileName))
		{
			bFileExist = true;
		}
#else
		IFileResolutionManager* pIFileResolutionManager = IFileResolutionManager::GetInstance();
		if(pIFileResolutionManager)
		{
			if(pIFileResolutionManager->GetFullFilePath(fileName, MaxSDK::AssetManagement::kXRefAsset))
			{
				bFileExist = true;
			}
			else if(pIFileResolutionManager->GetFullFilePath(fileName, MaxSDK::AssetManagement::kBitmapAsset))
			{
				bFileExist = true;
			}
		}		
#endif
	}

#ifdef UNICODE
	std::wstring fileName_w = fileName.GetCStr();
	std::string fileName_s;
	fileName_s.assign(fileName_w.begin(), fileName_w.end());
  fullPath = fileName_s;
#else
	fullPath = fileName.GetCStr();
#endif

  return bFileExist;
}

inline bool GetMaxFilePath(std::string file, std::string &fullPath)
{
  bool bFileExist = false;
#ifdef UNICODE
  std::string file_s = file.c_str();
	std::wstring file_w;
	file_w.assign(file_s.begin(), file_s.end());

  MaxSDK::Util::Path fileName(file_w.c_str());
  bFileExist = (DoesFileExist(file_w.c_str()) == TRUE) ? true : false;
#else
	MaxSDK::Util::Path fileName(file.c_str());
  bFileExist = (DoesFileExist(file.c_str()) == TRUE) ? true : false;
#endif
  if(!bFileExist)
	{
#ifdef PRE_MAX_2010
		if(IPathConfigMgr::GetPathConfigMgr()->SearchForXRefs(fileName))
		{
			bFileExist = true;
		}
#else
		IFileResolutionManager* pIFileResolutionManager = IFileResolutionManager::GetInstance();
		if(pIFileResolutionManager)
		{
			if(pIFileResolutionManager->GetFullFilePath(fileName, MaxSDK::AssetManagement::kXRefAsset))
			{
				bFileExist = true;
			}
			else if(pIFileResolutionManager->GetFullFilePath(fileName, MaxSDK::AssetManagement::kBitmapAsset))
			{
				bFileExist = true;
			}
		}		
#endif
	}

#ifdef UNICODE
	std::wstring fileName_w = fileName.GetCStr();
	std::string fileName_s;
	fileName_s.assign(fileName_w.begin(), fileName_w.end());
  fullPath = fileName_s;
#else
	fullPath = fileName.GetCStr();
#endif

  return bFileExist;
}

inline std::vector<std::string> ReadIFL(std::string path)
{
  std::string basePath = FilePath(path);
  std::vector<std::string> out;
  std::ifstream input(path);
  for(std::string line; getline(input, line);)
  {
    std::string ilfpath;
    input >> ilfpath;
    if (!ilfpath.empty())
    {
      ilfpath = basePath + ilfpath;
      out.push_back(ilfpath);
    }
  }

  return out;
}

#endif
