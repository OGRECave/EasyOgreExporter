////////////////////////////////////////////////////////////////////////////////
// materialSet.h
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

#ifndef _MATERIALSET_H
#define _MATERIALSET_H

#include "ExMaterial.h"
#include "ExPrerequisites.h"
#include "EasyOgreExporterLog.h"

namespace EasyOgreExporter
{
	class MaterialSet
	{
	public:
		//constructor
		MaterialSet()
    {
			//create a default material
			Material* defMat = new Material(0, "");
      m_materials.push_back(defMat);
		};

		//destructor
		~MaterialSet(){
		  clear();
		}

		//clear
		void clear(){
			for (int i=0; i<m_materials.size(); i++)
				delete m_materials[i];
			m_materials.clear();
		}

    Material* getMaterialByName(std::string name)
    {
			for (int i=0; i<m_materials.size(); i++)
			{
				if (m_materials[i]->name() == name)
					return m_materials[i];
			}
			return 0;
    }

		//add material
		void addMaterial(Material* pMat){
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
        std::string name = pMat->name();
        int index = 1;
        while(getMaterialByName(name))
        {
          std::stringstream strIdx;
          strIdx << index;
          name = pMat->name() + "_" + strIdx.str();
          
          index++;
        }
        
        if (index > 1)
          pMat->m_name = name;

				m_materials.push_back(pMat);
      }
		}

		//get material
		Material* getMaterial(IGameMaterial* pGameMaterial){
			for (int i=0; i<m_materials.size(); i++)
			{
				if (m_materials[i]->m_GameMaterial == pGameMaterial)
					return m_materials[i];
			}
			return NULL;
		};
		
		//write materials to Ogre XML
		bool writeOgreScript(ParamList &params)
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
				stat = m_materials[i]->writeOgreScript(params, outMaterial);
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
			return true;
		};

	protected:
		std::vector<Material*> m_materials;
	};

};	//end namespace

#endif
