////////////////////////////////////////////////////////////////////////////////
// material.cpp
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
// Author	      : Doug Perkowski - OC3 Entertainment, Inc.
// From work of : Francesco Giordana
// Start Date   : December 10th, 2007
////////////////////////////////////////////////////////////////////////////////

#include "ExMaterial.h"
#include "ExTools.h"
#include "EasyOgreExporterLog.h"
#include <imtl.h> 
#ifdef PRE_MAX_2010
#include "IPathConfigMgr.h"
#else
#include "IFileResolutionManager.h"
#endif	//PRE_MAX_2010 

namespace EasyOgreExporter
{
	// Constructor
  Material::Material(IGameMaterial* pGameMaterial, std::string prefix)
	{
    m_GameMaterial = pGameMaterial;
    m_name = getMaterialName(prefix);
		clear();
	}

	// Destructor
	Material::~Material()
	{
	}

	// Get material name
	std::string& Material::name()
	{
		return m_name;
	}

	// Clear data
	void Material::clear()
	{
		m_type = MT_LAMBERT;
		m_lightingOff = false;
		m_isTransparent = false;
		m_isTextured = false;
		m_ambient = Point4(1,1,1,1);
		m_diffuse = Point4(1,1,1,1);
		m_specular = Point4(0,0,0,1);
		m_emissive = Point4(0,0,0,1);
    m_opacity = 1.0f;
    m_shininess = 0.0f;
    m_isTwoSided = false;
    m_isWire = false;
    m_hasAlpha = false;
    m_hasAmbientMap = false;
    m_hasDiffuseMap = false;
    m_hasSpecularMap = false;
		m_textures.clear();
	}

  std::string Material::getMaterialName(std::string prefix)
  {
    std::string newMatName = "";

    if(!m_GameMaterial)
    {
      newMatName = "defaultLambert";
      return newMatName;
    }

    //read material name, adding the requested prefix
		std::string tmpStr = prefix;
		if (tmpStr != "")
			tmpStr += "/";

    std::string maxmat = m_GameMaterial->GetMaterialName();
    tmpStr.append(maxmat);

		for(size_t i = 0; i < tmpStr.size(); ++i)
		{
			if(tmpStr[i] == ':')
				newMatName.append("_");
			else
				newMatName.append(tmpStr.substr(i, 1));
		}

    return newMatName;
  }

	// Load material data
	bool Material::load(ParamList& params)
	{
		clear();
		//check if we want to export with lighting off option
		m_lightingOff = params.lightingOff;

		// GET MATERIAL DATA
		if(m_GameMaterial)
		{
			if(!m_GameMaterial->IsEntitySupported())
			{
        EasyOgreExporterLog("Warning: IsEntitySupported() returned false for IGameMaterial : %s...\n", m_GameMaterial->GetClassName());
			}
			else
			{
        IGameMaterial* pGameMaterial = m_GameMaterial;

        if(m_GameMaterial->IsMultiType() && (m_GameMaterial->GetSubMaterialCount() > 0))
          pGameMaterial = m_GameMaterial->GetSubMaterial(0);
        
        Mtl* maxMat = pGameMaterial->GetMaxMaterial();
        StdMat2* smat = 0;
        if(maxMat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
		    {
			    smat = static_cast<StdMat2*>(maxMat);
        }

        //Get extra material settings
        pGameMaterial->GetOpacityData()->GetPropertyValue(m_opacity, 0);
        if(m_opacity < 1.0f)
          m_isTransparent = true;

			  m_isTwoSided = smat->GetTwoSided();
		    m_isWire = smat->GetWire();
        if(smat->IsFaceted())
        {
          m_type = MT_FACETED;
        }
        else
        {
          if(smat->GetShading() == SHADE_PHONG)
            m_type = MT_PHONG;
        }

        if(!maxMat->GetSelfIllumColorOn())
        {
          IGameProperty* pGameProperty = pGameMaterial->GetEmissiveAmtData();
          if (IGAME_FLOAT_PROP == pGameProperty->GetType())
          {
            float amount = 0.0f;
            if(pGameProperty->GetPropertyValue(amount))
            {
				      m_emissive.x = amount;
				      m_emissive.y = amount;
				      m_emissive.z = amount;
				      m_emissive.w = m_opacity; 
            }
          }
        }
        else
        {
          exportColor(m_emissive, pGameMaterial->GetEmissiveData());
        }

        exportColor(m_ambient, pGameMaterial->GetAmbientData());
				exportColor(m_diffuse, pGameMaterial->GetDiffuseData());
				exportSpecular(pGameMaterial);

        // get material textures
				int texCount = pGameMaterial->GetNumberOfTextureMaps();
				EasyOgreExporterLog("Exporting %d textures...\n", texCount);
				for(int i = 0; i < texCount; ++i)
				{
					
					IGameTextureMap* pGameTexture = pGameMaterial->GetIGameTextureMap(i);
					if(pGameTexture && pGameTexture->IsEntitySupported())
					{
						EasyOgreExporterLog("Texture Index: %d\n",i);
						EasyOgreExporterLog("Texture Name: %s\n", pGameTexture->GetTextureName());

						Texture tex;
									
						MaxSDK::Util::Path textureName(pGameTexture->GetBitmapFileName());
						if(!DoesFileExist(pGameTexture->GetBitmapFileName()))
						{
							bool bFoundTexture = false;
#ifdef PRE_MAX_2010
							if(IPathConfigMgr::GetPathConfigMgr()->SearchForXRefs(textureName))
							{
								bFoundTexture = true;
							}
#else
							IFileResolutionManager* pIFileResolutionManager = IFileResolutionManager::GetInstance();
							if(pIFileResolutionManager)
							{
								if(pIFileResolutionManager->GetFullFilePath(textureName, MaxSDK::AssetManagement::kXRefAsset))
								{
									bFoundTexture = true;
								}
								else if(pIFileResolutionManager->GetFullFilePath(textureName, MaxSDK::AssetManagement::kBitmapAsset))
								{
									bFoundTexture = true;
								}
							}
#endif // PRE_MAX_2010
							if(true == bFoundTexture)
							{
								EasyOgreExporterLog("Updated texture location: %s.\n", textureName.GetCStr());
							}
							else
							{
								EasyOgreExporterLog("Warning: Couldn't locate texture: %s.\n", pGameTexture->GetTextureName());
							}				
						}
						int texSlot = pGameTexture->GetStdMapSlot();
						switch(texSlot)
						{
						case ID_AM:
              {
							  EasyOgreExporterLog("Ambient channel texture.\n");
                tex.bCreateTextureUnit = true;
                m_hasAmbientMap = true;
                
                BMMRES status; 
                BitmapInfo bi(tex.absFilename.c_str());
                Bitmap* bitmap = TheManager->Create(&bi); 
                bitmap = TheManager->Load(&bi, &status); 
                if (status == BMMRES_SUCCESS) 
                  if(bitmap->HasAlpha())
                    m_hasAlpha = true;

                TheManager->DelBitmap(bitmap);
                tex.type = ID_AM;
              }
							break;
						case ID_DI:
              {
							  EasyOgreExporterLog("Diffuse channel texture.\n");
							  tex.bCreateTextureUnit = true;
                m_hasDiffuseMap = true;
                
                BMMRES status; 
                BitmapInfo bi(tex.absFilename.c_str());
                Bitmap* bitmap = TheManager->Create(&bi); 
                bitmap = TheManager->Load(&bi, &status); 
                if (status == BMMRES_SUCCESS) 
                  if(bitmap->HasAlpha())
                    m_hasAlpha = true;

                TheManager->DelBitmap(bitmap);
                tex.type = ID_DI;
              }
							break;
						case ID_SP:
							EasyOgreExporterLog("Specular channel texture.\n");
              tex.bCreateTextureUnit = true;
              m_hasSpecularMap = true;
              tex.type = ID_SP;
							break;
						case ID_SH:
							EasyOgreExporterLog("SH channel texture.\n");
              tex.type = ID_SH;
							break;
						case ID_SS:
							EasyOgreExporterLog("Shininess Strenth channel texture.\n");
							tex.type = ID_SS;
              break;
						case ID_SI:
							EasyOgreExporterLog("Self-illumination channel texture.\n");
              tex.bCreateTextureUnit = true;
							tex.type = ID_SI;
              break;
						case ID_OP:
							EasyOgreExporterLog("opacity channel texture.\n");
              tex.bCreateTextureUnit = true;
              m_hasAlpha = true;
              tex.type = ID_OP;
							break;
						case ID_FI:
							EasyOgreExporterLog("Filter Color channel texture.\n");
							tex.type = ID_FI;
              break;
						case ID_BU:
							EasyOgreExporterLog("Bump channel texture.\n");
							tex.type = ID_BU;
              break;
						case ID_RL:
							EasyOgreExporterLog("Reflection channel texture.\n");
              tex.type = ID_RL;
              tex.bCreateTextureUnit = true;
              tex.bReflect = true;
							break; 
						case ID_RR:
							EasyOgreExporterLog("Refraction channel texture.\n");
							tex.type = ID_RR;
              break;
						case ID_DP:
							EasyOgreExporterLog("Displacement channel texture.\n");
							tex.type = ID_DP;
              break; 
						}
            
            //get the texture multiplier
            tex.fAmount = smat->GetTexmapAmt(texSlot, 0) * m_opacity;
            
						tex.absFilename = textureName.GetCStr();
						std::string filename = textureName.StripToLeaf().GetCStr();
						tex.filename = optimizeFileName(filename);
						tex.uvsetIndex = 0;
						tex.uvsetName = pGameTexture->GetTextureName();
            
						IGameUVGen *pUVGen = pGameTexture->GetIGameUVGen();
						IGameProperty *prop = NULL;
						if(pUVGen)
						{
							float covU,covV;
							prop = pUVGen->GetUTilingData();
							if(prop)
							{
								prop->GetPropertyValue(covU);
								if (fabs(covU) < PRECISION)
									covU = 666;
								else
									covU = 1/covU;
								tex.scale_u = covU;
								if (fabs(tex.scale_u) < PRECISION)
									tex.scale_u = 0;
							}
							prop = pUVGen->GetVTilingData();
							if(prop)
							{
								prop->GetPropertyValue(covV);
								if (fabs(covV) < PRECISION)
									covV = 666;
								else
									covV = 1/covV;
								tex.scale_v = covV;
								if (fabs(tex.scale_v) < PRECISION)
									tex.scale_v = 0;
							}
							float rot;
							prop = pUVGen->GetWAngleData();
							if(prop)
							{
								prop->GetPropertyValue(rot);
								tex.rot = -rot;
								// convert radians to degrees.
								tex.rot = tex.rot * 180.0f/3.1415926535f;
								if (fabs(rot) < PRECISION)
									tex.rot = 0;
							}

							float rotU, rotV;
							prop = pUVGen->GetUAngleData();
							if(prop)
							{
								prop->GetPropertyValue(rotU);
								if (fabs(rotU) > PRECISION)
									EasyOgreExporterLog("Warning: U rotation detected. This is unsupported.\n");
							}
							prop = pUVGen->GetVAngleData();
							{
								prop->GetPropertyValue(rotV);
								if (fabs(rotV) > PRECISION)
									EasyOgreExporterLog("Warning: V rotation detected. This is unsupported.\n");
							}

							float transU,transV;
							prop = pUVGen->GetUOffsetData();
							if(prop)
							{	
								prop->GetPropertyValue(transU);
								tex.scroll_u = -0.5 * (covU-1.0)/covU - transU/covU;
								if (fabs(tex.scroll_u) < PRECISION)
									tex.scroll_u = 0;
							}
							prop = pUVGen->GetVOffsetData();
							if(prop)
							{
								prop->GetPropertyValue(transV);
								tex.scroll_v = 0.5 * (covV-1.0)/covV + transV/covV;
								if (fabs(tex.scroll_v) < PRECISION)
									tex.scroll_v = 0;
							}

							tex.uvsetIndex = pGameTexture->GetMapChannel() - 1;

							m_textures.push_back(tex);
						}
					}
				}
			}
		}
		return true;
	}

	bool Material::exportColor(Point4& color, IGameProperty* pGameProperty)
	{
    if(IGAME_POINT4_PROP == pGameProperty->GetType())
		{
			Point4 matColor;
			if(pGameProperty->GetPropertyValue(matColor))
			{
        color = matColor;
  			return true;
			}
		}
		else if (IGAME_POINT3_PROP == pGameProperty->GetType())
		{
			Point3 color3;
			if(pGameProperty->GetPropertyValue(color3))
			{
				color.x = color3.x;
				color.y = color3.y;
				color.z = color3.z;
				color.w = m_opacity; 
				return true;
			}
		}

		return false;
	}
	
  bool Material::exportSpecular(IGameMaterial* pGameMaterial)
	{
    IGameProperty* pGameProperty = pGameMaterial->GetSpecularData();
		if(IGAME_POINT4_PROP == pGameProperty->GetType())
		{
			Point4 matColor;
			if(pGameProperty->GetPropertyValue(matColor))
			{
				m_specular = matColor;
		  }
		}
		else if (IGAME_POINT3_PROP == pGameProperty->GetType())
		{
			Point3 color3;
			if(pGameProperty->GetPropertyValue(color3))
			{
					m_specular.x = color3.x;
					m_specular.y = color3.y;
					m_specular.z = color3.z;
					m_specular.w = m_opacity; 
			}
		}
    else
    {
      return false;
    }

    //specular power
    float specularPower = 0.0f;
    if (pGameMaterial->GetSpecularLevelData() && pGameMaterial->GetSpecularLevelData()->GetType() == IGAME_FLOAT_PROP)
    {
      if(pGameMaterial->GetSpecularLevelData()->GetPropertyValue(specularPower))
        m_specular *= specularPower;
    }
    
    //specular glossiness
    float glossiness = 0.0;
    if (pGameMaterial->GetGlossinessData() && pGameMaterial->GetGlossinessData()->GetType() == IGAME_FLOAT_PROP)
    {
      pGameMaterial->GetGlossinessData()->GetPropertyValue(glossiness);
      m_shininess = 255.0f * glossiness;
    }

		return true;
	}

	// Write material data to an Ogre material script file
	bool Material::writeOgreScript(ParamList &params, std::ofstream &outMaterial)
	{
		//Start material description
		outMaterial << "material \"" << m_name.c_str() << "\"\n";
		outMaterial << "{\n";

    /*if(params.generateLOD)
    {
      outMaterial << "\tlod_strategy PixelCount\n";
      outMaterial << "\tlod_values 65000 8000 2000\n";

      for(int i = 0; i < 3; i++)
        writeMaterialTechnique(params, outMaterial, i);
    }
    else*/
    {
      writeMaterialTechnique(params, outMaterial, -1);
    }

		//End material description
		outMaterial << "}\n";

		//Copy textures to output dir if required
		if (params.copyTextures)
			copyTextures(params);

		return true;
	}

  void Material::writeMaterialTechnique(ParamList &params, std::ofstream &outMaterial, int lod)
  {
		//Start technique description
		outMaterial << "\ttechnique\n";
		outMaterial << "\t{\n";
    if(lod != -1)
      outMaterial << "\t\tlod_index "<< lod <<"\n";

    writeMaterialPass(params, outMaterial, lod);

		//End technique description
		outMaterial << "\t}\n";
  }

  void Material::writeMaterialPass(ParamList &params, std::ofstream &outMaterial, int lod)
  {
		//Start render pass description
		outMaterial << "\t\tpass\n";
		outMaterial << "\t\t{\n";
		//set lighting off option if requested
		if (m_lightingOff)
			outMaterial << "\t\t\tlighting off\n\n";
		
    if(m_isTwoSided)
    {
      outMaterial << "\t\t\tcull_hardware none\n";
      outMaterial << "\t\t\tcull_software none\n";
    }

    if(m_isWire)
      outMaterial << "\t\t\tpolygon_mode wireframe\n";
  
    //set phong shading if requested (default is gouraud)
    switch (m_type)
    {
      case MT_PHONG:
        outMaterial << "\t\t\tshading phong\n";
      break;

      case MT_FACETED:
        outMaterial << "\t\t\tshading flat\n";
      break;
    }
		
    //ambient colour
		// Format: ambient (<red> <green> <blue> [<alpha>]| vertexcolour)
    if(m_hasAmbientMap && lod == 0)
    {
      m_ambient.x = 1.0f;
      m_ambient.y = 1.0f;
      m_ambient.z = 1.0f;
    }
		outMaterial << "\t\t\tambient " << m_ambient.x << " " << m_ambient.y << " " << m_ambient.z << " " << m_ambient.w << "\n";
		
    //diffuse colour
		//Format: diffuse (<red> <green> <blue> [<alpha>]| vertexcolour)
    if(m_hasDiffuseMap && lod < 2)
    {
      m_diffuse.x = 1.0f;
      m_diffuse.y = 1.0f;
      m_diffuse.z = 1.0f;
    }
		outMaterial << "\t\t\tdiffuse " << m_diffuse.x << " " << m_diffuse.y << " " << m_diffuse.z << " " << m_diffuse.w << "\n";
		
    //specular colour and shininess
    if(m_hasSpecularMap && lod == 0)
    {
      m_specular.x = 1.0f;
      m_specular.y = 1.0f;
      m_specular.z = 1.0f;
    }
    outMaterial << "\t\t\tspecular " << m_specular.x << " " << m_specular.y << " " << m_specular.z	<< " " << m_specular.w << " " << m_shininess << "\n";
		
    //emissive colour
		outMaterial << "\t\t\temissive " << m_emissive.x << " " << m_emissive.y << " " << m_emissive.z << " " << m_emissive.w << "\n";
		
    //if material is transparent set blend mode and turn off depth_writing
		if (m_isTransparent)
		{
			outMaterial << "\n\t\t\tscene_blend alpha_blend\n";
			outMaterial << "\t\t\tdepth_write off\n";
		}

    if(m_hasAlpha)
			outMaterial << "\n\t\t\talpha_rejection greater 128\n";
		
    //write texture units
    //TODO manage ambient, diffuse, spec, alpha layer types with colour_op_ex <operation> <source1> <source2> [<manual_factor>] [<manual_colour1>] [<manual_colour2>]
		if(lod < 2) // no texture for last LOD
    {
      for (int i=0; i<m_textures.size(); i++)
		  {
			  if(m_textures[i].bCreateTextureUnit == true)
			  {
          //only diffuse on next lod
          if((m_textures[i].type != ID_DI) && (lod > 0))
            continue;

				  //start texture unit description
				  outMaterial << "\n\t\t\ttexture_unit\n";
				  outMaterial << "\t\t\t{\n";
				  //write texture name
				  outMaterial << "\t\t\t\ttexture " << m_textures[i].filename.c_str() << "\n";
				  //write texture coordinate index
				  outMaterial << "\t\t\t\ttex_coord_set " << m_textures[i].uvsetIndex << "\n";

          /*
          switch (m_textures[i].type)
          {
          case ID_DI:
            outMaterial << "\t\t\t\tcolour_op_ex modulate src_texture src_diffuse " << m_textures[i].fAmount << "\n";
          break;

          case ID_SP:
            outMaterial << "\t\t\t\tcolour_op_ex modulate src_texture src_specular " << m_textures[i].fAmount << "\n";
          break;

          default:
            if(m_textures[i].fAmount >= 1.0f)
            {
              outMaterial << "\t\t\t\tcolour_op modulate\n";
            }
            else
            {
              outMaterial << "\t\t\t\tcolour_op_ex blend_manual src_texture src_current " << m_textures[i].fAmount << "\n";
              outMaterial << "\t\t\t\tcolour_op_multipass_fallback one zero\n";
            }
          }*/

          if(m_textures[i].fAmount >= 1.0f)
          {
            outMaterial << "\t\t\t\tcolour_op modulate\n";
          }
          else
          {
            outMaterial << "\t\t\t\tcolour_op_ex blend_manual src_texture src_current " << m_textures[i].fAmount << "\n";
            outMaterial << "\t\t\t\tcolour_op_multipass_fallback one zero\n";
          }

          //write colour operation
				  /*
          switch (m_textures[i].opType)
				  {
				  case TOT_REPLACE:
					  outMaterial << "\t\t\t\tcolour_op replace\n";
					  break;
				  case TOT_ADD:
					  outMaterial << "\t\t\t\tcolour_op add\n";
					  break;
				  case TOT_MODULATE:
					  outMaterial << "\t\t\t\tcolour_op modulate\n";
					  break;
				  case TOT_ALPHABLEND:
					  outMaterial << "\t\t\t\tcolour_op alpha_blend\n";
					  break;
          case TOT_MANUALBLEND:
            outMaterial << "\t\t\t\tcolour_op_ex blend_manual src_texture src_current " << m_textures[i].fAmount << "\n";
            outMaterial << "\t\t\t\tcolour_op_multipass_fallback one zero\n";
					  break;
				  }
          */

				  //write texture transforms
				  outMaterial << "\t\t\t\tscale " << m_textures[i].scale_u << " " << m_textures[i].scale_v << "\n";
				  outMaterial << "\t\t\t\tscroll " << m_textures[i].scroll_u << " " << m_textures[i].scroll_v << "\n";
				  outMaterial << "\t\t\t\trotate " << m_textures[i].rot << "\n";

          if(m_textures[i].bReflect)
            outMaterial << "\t\t\t\tenv_map " << "cubic_reflection" << "\n";

				  //end texture unit desription
				  outMaterial << "\t\t\t}\n";
			  }
		  }
    }

		//End render pass description
		outMaterial << "\t\t}\n";
  }

	// Copy textures to path specified by params
	bool Material::copyTextures(ParamList &params)
	{
		for (int i=0; i<m_textures.size(); i++)
		{
      std::string destFile = makeOutputPath(params.outputDir, params.texOutputDir, m_textures[i].filename, "");

			// Copy file texture to output dir
      if(!CopyFile(m_textures[i].absFilename.c_str(), destFile.c_str(), false))
        EasyOgreExporterLog("Error while copying texture file %s\n", m_textures[i].absFilename.c_str());
		}
		return true;
	}

};	//end namespace
