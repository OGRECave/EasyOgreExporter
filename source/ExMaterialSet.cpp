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

		for (int i=0; i<m_vsShaders.size(); i++)
			delete m_vsShaders[i];
		m_vsShaders.clear();

	 for (int i=0; i<m_fpShaders.size(); i++)
			delete m_fpShaders[i];
		m_fpShaders.clear();
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

  void ExMaterialSet::addVsShader(ExVsShader* vsShader)
  {
    m_vsShaders.push_back(vsShader);
  };

  ExVsShader* ExMaterialSet::getVsShader(std::string& name)
  {
		ExVsShader* vsShader = 0;
		for (int i=0; i<m_vsShaders.size() && !vsShader; i++)
		{
			if (m_vsShaders[i]->getName() == name)
			{
				vsShader = m_vsShaders[i];
			}
		}
    return vsShader;
  };

  void ExMaterialSet::addFpShader(ExFpShader* fpShader)
  {
    m_fpShaders.push_back(fpShader);
  };

  ExFpShader* ExMaterialSet::getFpShader(std::string& name)
  {
		ExFpShader* fpShader = 0;
		for (int i=0; i<m_fpShaders.size() && !fpShader; i++)
		{
			if (m_fpShaders[i]->getName() == name)
			{
				fpShader = m_fpShaders[i];
			}
		}
    return fpShader;
  };

  ExVsShader* ExMaterialSet::createVsShader(ExMaterial* mat)
  {
    std::string sname = mat->getVsShaderName();
    ExVsShader* vsShader = getVsShader(sname);
    if(vsShader)
      return vsShader;
    
    vsShader = new ExVsShader(sname, mat);
    addVsShader(vsShader);
    return vsShader;
  }

  ExFpShader* ExMaterialSet::createFpShader(ExMaterial* mat)
  {
    std::string sname = mat->getFpShaderName();
    ExFpShader* fpShader = getFpShader(sname);
    if(fpShader)
      return fpShader;
    
    fpShader = new ExFpShader(sname, mat);
    addFpShader(fpShader);
    return fpShader;
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
      ExVsShader* vsShader = params.exportProgram ? createVsShader(m_materials[i]) : 0;
      ExFpShader* fpShader = params.exportProgram ? createFpShader(m_materials[i]) : 0;
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

      for (int i = 0; i < m_vsShaders.size(); i++)
      {
        outShaderCG << m_vsShaders[i]->getContent();
        outShaderCG << "\n";

        outProgram << m_vsShaders[i]->getProgram(params.sceneFilename);
        outProgram << "\n";
      }

      for (int i = 0; i < m_fpShaders.size(); i++)
      {
        outShaderCG << m_fpShaders[i]->getContent();
        outShaderCG << "\n";

        outProgram << m_fpShaders[i]->getProgram(params.sceneFilename);
        outProgram << "\n";
      }

      outShaderCG.close();
      outProgram.close();
    }
		return true;
	};

};	//end namespace
