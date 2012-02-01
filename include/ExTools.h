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

  // dummy type
  if(os.obj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0))
    return true;

  // bided
  Control *cont = pNode->GetTMController();   
  if(cont->ClassID() == BIPSLAVE_CONTROL_CLASS_ID ||       //others biped parts    
          cont->ClassID() == BIPBODY_CONTROL_CLASS_ID)     //biped root "Bip01"     
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
  Control *cont = pNode->GetTMController();
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

// Removes scaling from originalMatrix.
inline Matrix3 RemoveNonUniformScaling(const Matrix3& originalMatrix)
{          
	AffineParts matrixParts;
	decomp_affine(originalMatrix, &matrixParts);
	Matrix3 nodeMatrixNoScale;
	matrixParts.q.MakeMatrix(nodeMatrixNoScale);
	nodeMatrixNoScale.SetTrans(matrixParts.t);
	return nodeMatrixNoScale;
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

inline Matrix3 GetRelativeMatrix(Matrix3 mat1, Matrix3 mat2, bool yUp)
{          
  Matrix3 dest_mat;
  
  //Decompose each matrix
  Matrix3 cur_mat = UniformMatrix(mat1, yUp);
  Matrix3 par_mat = UniformMatrix(mat2, yUp);
 
  //then return relative matrix in coordinate space of parent
  dest_mat = cur_mat * Inverse(par_mat);

  return dest_mat;
}

inline Matrix3 GetRelativeMatrix(INode *node, int t, bool yUp)
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

#endif
