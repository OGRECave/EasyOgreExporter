////////////////////////////////////////////////////////////////////////////////
// ExMaterialSet.h
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
// Author	      : Francesco Giordana
// Start Date   : December 10th, 2007
////////////////////////////////////////////////////////////////////////////////

#include "ExMaterialSet.h"

namespace EasyOgreExporter
{
  ExMaterialSet::ExMaterialSet()
  {
		//create a default material
		ExMaterial* defMat = new ExMaterial(0, "");
    m_materials.push_back(defMat);
	};

	//destructor
	ExMaterialSet::~ExMaterialSet()
  {
	  clear();
	};

	//clear
	void ExMaterialSet::clear()
  {
		for (int i=0; i<m_materials.size(); i++)
			delete m_materials[i];
		m_materials.clear();

		for (int i=0; i<m_Shaders.size(); i++)
			delete m_Shaders[i];
		m_Shaders.clear();
	};

  ExMaterial* ExMaterialSet::getMaterialByName(std::string name)
  {
		for (int i=0; i<m_materials.size(); i++)
		{
      if (m_materials[i]->getName() == name)
				return m_materials[i];
		}
		return 0;
  };

  void ExMaterialSet::addShader(ExShader* shader)
  {
    m_Shaders.push_back(shader);
  };

  ExShader* ExMaterialSet::getShader(std::string& name)
  {
		ExShader* shader = 0;
		for (int i=0; i<m_Shaders.size() && !shader; i++)
		{
			if (m_Shaders[i]->getName() == name)
			{
				shader = m_Shaders[i];
			}
		}
    return shader;
  };

  ExShader* ExMaterialSet::createShader(ExMaterial* mat, ExShader::ShaderType type)
  {
    std::string sname = mat->getShaderName(type);
    ExShader* shader = getShader(sname);
    if(shader)
      return shader;
    
    switch (type)
    {
      case ExShader::ST_VSLIGHT :
        shader = new ExVsShader(sname);
      break;

      case ExShader::ST_FPLIGHT :
        shader = new ExFpShader(sname);
      break;
    }

    if(shader)
    {
      shader->constructShader(mat);
      addShader(shader);
    }
    return shader;
  }

	//add material
	void ExMaterialSet::addMaterial(ExMaterial* pMat)
  {
		bool found = false;
		for (int i=0; i<m_materials.size() && !found; i++)
		{
			if (m_materials[i]->m_GameMaterial == pMat->m_GameMaterial)
			{
				found = true;
				delete pMat;
			}
		}
		if (!found)
    {
      //be aware of duplicated materials names
      std::string name = pMat->getName();
      int index = 1;
      while(getMaterialByName(name))
      {
        std::stringstream strIdx;
        strIdx << index;
        name = pMat->getName() + "_" + strIdx.str();
        
        index++;
      }
      
      if (index > 1)
        pMat->getName() = name;

			m_materials.push_back(pMat);
    }
	};

	//get material
	ExMaterial* ExMaterialSet::getMaterial(IGameMaterial* pGameMaterial)
  {
		for (int i=0; i<m_materials.size(); i++)
		{
			if (m_materials[i]->m_GameMaterial == pGameMaterial)
				return m_materials[i];
		}
		return NULL;
	};
	
	//write materials to Ogre Script
	bool ExMaterialSet::writeOgreScript(ParamList &params)
  {
		bool stat = false;
    std::string msg;

    std::ofstream outMaterial;
	  
    std::string filePath = makeOutputPath(params.outputDir, params.materialOutputDir, params.sceneFilename, "material");
	  outMaterial.open(filePath.c_str());
	  if (!outMaterial)
	  {
			EasyOgreExporterLog("Error opening file: %s\n", filePath.c_str());
		  return false;
	  }

		for (int i=0; i<m_materials.size(); i++)
		{
      ExShader* vsShader = params.exportProgram ? createShader(m_materials[i], ExShader::ST_VSLIGHT) : 0;
      ExShader* fpShader = params.exportProgram ? createShader(m_materials[i], ExShader::ST_FPLIGHT) : 0;
			stat = m_materials[i]->writeOgreScript(params, outMaterial, vsShader ,fpShader);

			/*
			if (true != stat)
			{
				std::string msg = "Error writing material ";
				msg += m_materials[i]->name();
				msg += ", aborting operation";
				MGlobal::displayInfo(msg);
			}
			*/
		}
    outMaterial.close();

    if(params.exportProgram)
    {
      std::ofstream outShaderCG;
      std::ofstream outProgram;
  	  
      std::string cgFilePath = makeOutputPath(params.outputDir, params.programOutputDir, params.sceneFilename, "cg");
	    outShaderCG.open(cgFilePath.c_str());
	    if (!outShaderCG)
	    {
			  EasyOgreExporterLog("Error opening file: %s\n", cgFilePath.c_str());
		    return false;
	    }
      
      std::string prFilePath = makeOutputPath(params.outputDir, params.programOutputDir, params.sceneFilename, "program");
	    outProgram.open(prFilePath.c_str());
	    if (!outProgram)
	    {
			  EasyOgreExporterLog("Error opening file: %s\n", prFilePath.c_str());
		    return false;
	    }

      for (int i = 0; i < m_Shaders.size(); i++)
      {
        outShaderCG << m_Shaders[i]->getContent();
        outShaderCG << "\n";

        outProgram << m_Shaders[i]->getProgram(params.sceneFilename);
        outProgram << "\n";
      }

      outShaderCG.close();
      outProgram.close();
    }
		return true;
	};

};	//end namespace
