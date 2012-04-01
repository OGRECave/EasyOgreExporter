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

inline void trim(std::string& str)
{
  std::string::size_type pos = str.find_last_not_of(' ');
  if(pos != std::string::npos)
  {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if(pos != std::string::npos)
      str.erase(0, pos);
  }
  else
    str.erase(str.begin(), str.end());
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

// Helper function to replace special chars for file names
inline std::string optimizeFileName(const std::string& filename)
{
  std::string newFilename = filename;
  std::replace(newFilename.begin(), newFilename.end(), ':', '_');
  std::replace(newFilename.begin(), newFilename.end(), ' ', '_');
  std::replace(newFilename.begin(), newFilename.end(), '\'', '_');
  std::replace(newFilename.begin(), newFilename.end(), '/', '_');
  std::replace(newFilename.begin(), newFilename.end(), '"', '_');
	return newFilename;;
}

// Helper function to replace special chars for resources
inline std::string optimizeResourceName(const std::string& filename)
{
  std::string newFilename = filename;
  std::replace(newFilename.begin(), newFilename.end(), ':', '_');
  std::replace(newFilename.begin(), newFilename.end(), ' ', '_');
  std::replace(newFilename.begin(), newFilename.end(), '\'', '_');
  std::replace(newFilename.begin(), newFilename.end(), '"', '_');
	return newFilename;;
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


inline bool IsBone(INode *pNode)
{
  if(pNode == NULL)
    return false; 

  ObjectState os = pNode->EvalWorldState(0); 
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

  ObjectState os = pNode->EvalWorldState(0); 
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

inline bool IsBipedRoot(INode *pNode)
{
  if(pNode == NULL)
    return false; 

  ObjectState os = pNode->EvalWorldState(0); 
  if (!os.obj)
    return false;

  // bided
  Control* cont = pNode->GetTMController();   
  if(cont->ClassID() == BIPBODY_CONTROL_CLASS_ID)   //biped root "Bip01"
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

inline bool IsRootBone(INode *pNode)
{
  if(pNode == NULL)
    return false; 

  ObjectState os = pNode->EvalWorldState(0); 
  if (!os.obj) 
    return false;

  //
  Control *cont = pNode->GetTMController();   
  if(cont->ClassID() == BIPBODY_CONTROL_CLASS_ID)     //biped root "Bip01"     
    return true;

  return false;   
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

inline Matrix3 GetRelativeUniformMatrix(Matrix3 mat1, Matrix3 mat2, bool yUp)
{          
  Matrix3 dest_mat;
  
  //Decompose each matrix
  Matrix3 cur_mat = UniformMatrix(mat1, yUp);
  Matrix3 par_mat = UniformMatrix(mat2, yUp);
 
  //then return relative matrix in coordinate space of parent
  dest_mat = cur_mat * Inverse(par_mat);

  return dest_mat;
}

inline Matrix3 GetRelativeUniformMatrix(INode *node, int t, bool yUp)
{          
 /* Note: This function removes the non-uniform scaling 
 from MAX node transformations. before multiplying the 
 current node by  the inverse of its parent. The 
 removal  must be done on both nodes before the 
 multiplication and Inverse are applied. This is especially 
 useful for Biped export (which uses non-uniform scaling on 
 its body parts.) */
 
  INode *p_node = node->GetParentNode();
 
  Matrix3 orig_cur_mat;  // for current and parent 
  Matrix3 orig_par_mat;  // original matrices 
 
  Matrix3 cur_mat;       // for current and parent
  Matrix3 par_mat;       // decomposed matrices 

  Matrix3 dest_mat;
  
  //Get transformation matrices
  orig_cur_mat = node->GetNodeTM(t);
  orig_par_mat = p_node->GetNodeTM(t); 
  
  //Decompose each matrix
  cur_mat = UniformMatrix(orig_cur_mat, yUp);
  par_mat = UniformMatrix(orig_par_mat, yUp);
 
  //then return relative matrix in coordinate space of parent
  dest_mat = cur_mat * Inverse(par_mat);

  return dest_mat;
}

inline Matrix3 GetRelativeMatrix(INode *node, int t, bool yUp)
{
  INode *p_node = node->GetParentNode();
 
  Matrix3 orig_cur_mat;  // for current and parent 
  Matrix3 orig_par_mat;  // original matrices 
 
  Matrix3 cur_mat;       // for current and parent
  Matrix3 par_mat;       // decomposed matrices 

  Matrix3 dest_mat;
  
  //Get transformation matrices
  orig_cur_mat = node->GetNodeTM(t);
  orig_par_mat = p_node->GetNodeTM(t); 
  
  //Decompose each matrix
  cur_mat = TransformMatrix(orig_cur_mat, yUp);
  par_mat = TransformMatrix(orig_par_mat, yUp);
 
  //then return relative matrix in coordinate space of parent
  dest_mat = cur_mat * Inverse(par_mat);

  return dest_mat;
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

inline float ConvertToMeter(DispInfo disp, int unitType)
{
  float scale = GetUnitValue(unitType);
  
  if(disp.dispType == UNITDISP_METRIC)
  switch(disp.metricDisp)
  {
    case UNIT_METRIC_DISP_MM:
      scale = 1000.0f * GetUnitValue(unitType);
    break;

    case UNIT_METRIC_DISP_CM:
      scale = 100.0f * GetUnitValue(unitType);
    break;

    case UNIT_METRIC_DISP_M:
      scale = 1.0f * GetUnitValue(unitType);
    break;

    case UNIT_METRIC_DISP_KM:
      scale = 0.001f * GetUnitValue(unitType);
    break;
  }

  return scale;
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

  if(pGameControl->IsAnimated(IGAME_POS))
  {
    if(!GetAnimationsPosKeysTime(pGameControl, animRange, &animKeys))
    {
      IGameKeyTab poskeys;
      if(pGameControl->GetFullSampledKeys(poskeys, 1, IGameControlType(IGAME_TM), true))
      {
        AddKeyTabToVector(poskeys, animRange, &animKeys);
        return animKeys;
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

        std::sort(animKeys.begin(), animKeys.end());
        animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
        return animKeys;
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

        std::sort(animKeys.begin(), animKeys.end());
        animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
        return animKeys;
      }
    }
  }

  //sort and remove duplicated entries
  if(animKeys.size() > 0)
  {
    std::sort(animKeys.begin(), animKeys.end());
    animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
  }

  return animKeys;
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

  if(pGameControl->IsAnimated(IGAME_POINT3))
  {
    if(!GetAnimationsPointKeysTime(pGameControl, animRange, &animKeys))
    {
      //add time steps
      for (int t = animRange.Start(); t < animRange.End(); t += animRate)
        animKeys.push_back(t);

      //force the last key
      animKeys.push_back(animRange.End());
      animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
      return animKeys;
    }
  }

  //sort and remove duplicated entries
  if(animKeys.size() > 0)
  {
    std::sort(animKeys.begin(), animKeys.end());
    animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());
  }

  return animKeys;
}

#endif
