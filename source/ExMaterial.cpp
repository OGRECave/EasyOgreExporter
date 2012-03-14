////////////////////////////////////////////////////////////////////////////////
// ExMaterial.cpp
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

/*
  TODO
  shader
  name = VREFTEX123.. FAMB1DIFF0NORM2...
  TEXCOORD list from material
  texture type TEXCOORD

  "sampler"
  ambient or not
  diffuse or not
  lightmap or not
  specular or not
  normal or not
  alpha or not
  refmap or not (IGNORE TEXCOORD for this one)
  displacement or not
  
   2 uv coords max for the heavy case
    float4 wp   : TEXCOORD1;
    float3 n    : TEXCOORD2;
    float3 t    : TEXCOORD3;
    float3 b    : TEXCOORD4;
    float3 sdir : TEXCOORD5;
    float3 texProj : TEXCOORD6;

  special cases :
  car shader
  cartoon

  1 contruct material
  2 determine vertex shader
  2 determine pixel shader
  3 get or construct shaders
  4 store shaders in materialset
  5 construct program / determine profil type
  6 write shaders after material script
*/

namespace EasyOgreExporter
{
  MatProc::MatProc()
  {
  }

  bool MatProc::Proc(IGameProperty * prop)
  {
    EasyOgreExporterLog("Debug: Material properties : %s\n", prop->GetName());
    return false;//lets keep going
  }


	// Constructor
  ExMaterial::ExMaterial(IGameMaterial* pGameMaterial, std::string prefix)
	{
    m_GameMaterial = pGameMaterial;
    m_name = getMaterialName(prefix);
		clear();
	}

	// Destructor
	ExMaterial::~ExMaterial()
	{
	}

	// Get material name
	std::string& ExMaterial::getName()
	{
		return m_name;
	}

	// Clear data
	void ExMaterial::clear()
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
    m_reflectivity = 0.0f;
    m_normalMul = 1.0f;
    m_isTwoSided = false;
    m_isWire = false;
    m_hasAlpha = false;
    m_hasAmbientMap = false;
    m_hasDiffuseMap = false;
    m_hasSpecularMap = false;
    m_hasReflectionMap = false;
    m_hasBumpMap = false;
		m_textures.clear();
	}

  std::string ExMaterial::getMaterialName(std::string prefix)
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
    trim(maxmat);
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

  std::string ExMaterial::getShaderName(ExShader::ShaderType type)
  {
    std::string sname;
    std::stringstream out;

    switch (type)
    {
      case ExShader::ST_VSAM :
      {
        out << "vsAmbGEN";

        std::vector<int> texUnits;
        for (int i=0; i<m_textures.size(); i++)
        {
          if(m_textures[i].bCreateTextureUnit == true)
          {
            switch (m_textures[i].type)
            {
              case ID_AM:
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_DI:
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_SI:
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_RL:
                out << "REF";
              break;
            }
          }
        }
        std::sort(texUnits.begin(), texUnits.end());
        texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());

        for (int i=0; i<texUnits.size(); i++)
        {
          out << texUnits[i];
        }
        break;
      }

      case ExShader::ST_FPAM:
      {
        out << "fpAmbGEN";

        for (int i=0; i<m_textures.size(); i++)
        {
	        if(m_textures[i].bCreateTextureUnit == true)
	        {
            switch (m_textures[i].type)
            {
              case ID_AM:
                out << "AMB";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_DI:
                out << "DIFF";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_SI:
                out << "LM";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_RL:
                out << "REF";
              break;
            }
          }
        }
        break;
      }

      case ExShader::ST_VSLIGHT :
      {
        out << "vsLightGEN";

        std::vector<int> texUnits;
        for (int i=0; i<m_textures.size(); i++)
        {
          if(m_textures[i].bCreateTextureUnit == true)
          {
            switch (m_textures[i].type)
            {
              case ID_AM:
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_DI:
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_SP:
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_BU:
                out << "NORM";
                texUnits.push_back(m_textures[i].uvsetIndex);
              break;

              case ID_RL:
              break;
            }
          }
        }
        std::sort(texUnits.begin(), texUnits.end());
        texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());

        for (int i=0; i<texUnits.size(); i++)
        {
          out << texUnits[i];
        }
        break;
      }

      case ExShader::ST_FPLIGHT:
      {
        out << "fpLightGEN";

        for (int i=0; i<m_textures.size(); i++)
        {
	        if(m_textures[i].bCreateTextureUnit == true)
	        {
            switch (m_textures[i].type)
            {
              case ID_AM:
                out << "AMB";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_DI:
                out << "DIFF";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_SP:
                out << "SPEC";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_BU:
                out << "NORM";
                out << m_textures[i].uvsetIndex;
              break;

              case ID_RL:
                out << "REF";
              break;
            }
          }
        }
        break;
      }
    }

    sname = out.str();
    return sname;
  }

  void ExMaterial::loadTextureUV(IGameTextureMap* pGameTexture, Texture &tex)
  {
    if(!pGameTexture)
      return;

    IGameUVGen* pUVGen = pGameTexture->GetIGameUVGen();
		IGameProperty* prop = NULL;
		if(pUVGen)
		{
			float covU, covV;
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
    }
  }

  void ExMaterial::loadManualTexture(IGameProperty* prop, int type, float amount)
  {
    if(prop)
    {
      if (prop->IsParamBlock())
			{
				if (prop->IsPBlock2())
				{
        #ifdef PRE_MAX_2012
					int index = prop->GetParamBlockIndex();
        #elif PRE_MAX_2011
          int index = prop->GetParamBlockIndex();
        #else
          int index = prop->GetParamIndex();
        #endif

					IParamBlock2* pBlock = prop->GetMaxParamBlock2();
					ParamID id = pBlock->IndextoID (index);
					Texmap* pTexmap = pBlock->GetTexmap (id);
					if (pTexmap)
					{
						MSTR className;
						pTexmap->GetClassName(className);
						const char* pName = className.data();
						if (!strcmp(pName, "Bitmap"))
						{
							BitmapTex* pBitmapTex = static_cast<BitmapTex*>(pTexmap);
              std::string path = pBitmapTex->GetMapName();

              EasyOgreExporterLog("Texture Name: %s\n", path.c_str());
			        
              bool bFoundTexture = false;
		          MaxSDK::Util::Path textureName(path.c_str());
		          if(!DoesFileExist(path.c_str()))
		          {
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
				          EasyOgreExporterLog("Warning: Couldn't locate texture: %s.\n", path);
			          }
		          }
                            
              Texture tex;
		          tex.bCreateTextureUnit = true;
		          tex.uvsetIndex = 0;
              tex.uvsetIndex = pBitmapTex->GetMapChannel() - 1;
              tex.uvsetName = pBitmapTex->GetName();
			        tex.absFilename = textureName.GetCStr();
			        std::string filename = textureName.StripToLeaf().GetCStr();
			        tex.filename = optimizeFileName(filename);
              tex.fAmount = amount;

              if(bFoundTexture)
              {
                BMMRES status;
                BitmapInfo bi(tex.absFilename.c_str());
                Bitmap* bitmap = TheManager->Create(&bi); 
                bitmap = TheManager->Load(&bi, &status);
                if (status == BMMRES_SUCCESS) 
                  if(bitmap->HasAlpha())
                    m_hasAlpha = true;

                TheManager->DelBitmap(bitmap);
                tex.type = type;
              }

              //retrieve the IGameTextureMap
              IGameTextureMap* pGameTexture = GetIGameInterface()->GetIGameTextureMap(pTexmap);
              loadTextureUV(pGameTexture, tex);

              m_textures.push_back(tex);
						}
					}
				}
			}
    }
  }

  void ExMaterial::loadArchAndDesignMaterial(IGameMaterial* pGameMaterial)
  {
    IPropertyContainer* pCont = pGameMaterial->GetIPropertyContainer();

    Point4 transColor;
    IGameProperty* pTransColor = pCont->QueryProperty(_T("refr_color"));
    if(pTransColor)
    {
      pTransColor->GetPropertyValue(transColor);
    }

    IGameProperty* pOpacity = pCont->QueryProperty(_T("refr_weight"));
    if(pOpacity)
    {
      pOpacity->GetPropertyValue(m_opacity);
      m_opacity = 1.0f - (m_opacity * ((transColor.x + transColor.y + transColor.z) / 3.0f));
      if(m_opacity < 1.0f)
        m_isTransparent = true;
    }

    float glossiness = 0.0f;
    IGameProperty* pShininess = pCont->QueryProperty(_T("refl_weight"));
    if(pShininess)
    {
      pShininess->GetPropertyValue(glossiness);
      m_shininess = 255.0f * glossiness;
    }

    //TODO
    float luminance = 0.0f;
    IGameProperty* pLuminance = pCont->QueryProperty(_T("luminance"));
    if(pLuminance)
    {
      pLuminance->GetPropertyValue(luminance);
      //3000 seems to be the max viewport value
      luminance = luminance / 3000;
      m_emissive.x = luminance;
      m_emissive.y = luminance;
      m_emissive.z = luminance;
      m_emissive.w = m_opacity;
    }

    IGameProperty* pDiffuse = pCont->QueryProperty(_T("diff_color"));
    if(pDiffuse)
    {
      Point4 diffuse;
      pDiffuse->GetPropertyValue(diffuse);
      m_diffuse.x = diffuse.x;
      m_diffuse.y = diffuse.y;
		  m_diffuse.z = diffuse.z;
		  m_diffuse.w = m_opacity;

      m_ambient.x = diffuse.x;
      m_ambient.y = diffuse.y;
		  m_ambient.z = diffuse.z;
		  m_ambient.w = m_opacity;
    }
    
    IGameProperty* pSpecular = pCont->QueryProperty(_T("refl_color"));
    if(pSpecular)
    {
      Point4 specular;
      pSpecular->GetPropertyValue(specular);
      m_specular.x = specular.x;
      m_specular.y = specular.y;
		  m_specular.z = specular.z;
		  m_specular.w = m_opacity;
    }

    float diffAmount = 1.0f;
    IGameProperty* pDiffuseMul = pCont->QueryProperty(_T("diff_weight"));
    if(pDiffuseMul)
    {
      pDiffuseMul->GetPropertyValue(diffAmount);
    }

    IGameProperty* pDiffuseMap = pCont->QueryProperty(_T("diff_color_map"));
    loadManualTexture(pDiffuseMap, ID_DI, diffAmount);
    m_hasDiffuseMap = true;
  }

  void ExMaterial::loadArchitectureMaterial(IGameMaterial* pGameMaterial)
  {
    IPropertyContainer* pCont = pGameMaterial->GetIPropertyContainer();
    IGameProperty* pOpacity = pCont->QueryProperty(_T("transparency"));
    if(pOpacity)
    {
      pOpacity->GetPropertyValue(m_opacity);
      m_opacity = 1.0f - m_opacity;
      if(m_opacity < 1.0f)
        m_isTransparent = true;
    }

    int twoSided = 0;
    IGameProperty* pTwoSided = pCont->QueryProperty(_T("twoSided"));
    if(pTwoSided)
    {
      pTwoSided->GetPropertyValue(twoSided);
      m_isTwoSided = twoSided ? true : false;
    }

    float glossiness = 0.0f;
    IGameProperty* pShininess = pCont->QueryProperty(_T("shininess"));
    if(pShininess)
    {
      pShininess->GetPropertyValue(glossiness);
      m_shininess = 255.0f * glossiness;
    }

    float luminance = 0.0f;
    IGameProperty* pLuminance = pCont->QueryProperty(_T("luminance"));
    if(pLuminance)
    {
      pLuminance->GetPropertyValue(luminance);
      //3000 seems to be the max viewport value
      luminance = luminance / 3000;
      m_emissive.x = luminance;
      m_emissive.y = luminance;
      m_emissive.z = luminance;
      m_emissive.w = m_opacity;
    }

    IGameProperty* pDiffuse = pCont->QueryProperty(_T("diffuse"));
    if(pDiffuse)
    {
      Point3 diffuse;
      pDiffuse->GetPropertyValue(diffuse);
      m_diffuse.x = diffuse.x;
      m_diffuse.y = diffuse.y;
		  m_diffuse.z = diffuse.z;
		  m_diffuse.w = m_opacity;

      m_ambient.x = diffuse.x;
      m_ambient.y = diffuse.y;
		  m_ambient.z = diffuse.z;
		  m_ambient.w = m_opacity;
    }
    
    float reflectance = 0.0f;
    IGameProperty* pReflectance = pCont->QueryProperty(_T("reflectanceScale"));
    if(pReflectance)
    {
      pReflectance->GetPropertyValue(reflectance);
      m_specular.x = 1.0f * (reflectance / 2.5f);
      m_specular.y = 1.0f * (reflectance / 2.5f);
		  m_specular.z = 1.0f * (reflectance / 2.5f);
		  m_specular.w = m_opacity;
    }
    
    float diffAmount = 1.0f;
    IGameProperty* pDiffuseMul = pCont->QueryProperty(_T("diffuseMapAmount"));
    if(pDiffuseMul)
    {
      pDiffuseMul->GetPropertyValue(diffAmount);
    }

    IGameProperty* pDiffuseMap = pCont->QueryProperty(_T("diffuseMap"));
    loadManualTexture(pDiffuseMap, ID_DI, diffAmount);
    m_hasDiffuseMap = true;
  }

  void ExMaterial::loadStandardMaterial(IGameMaterial* pGameMaterial)
  {
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

    m_isTwoSided = smat->GetTwoSided() ? true : false;
    m_isWire = smat->GetWire() ? true : false;
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
      std::string texClass = pGameTexture->GetClassName();

      EasyOgreExporterLog("Exporting %d texture from %s...\n", i, texClass.c_str());
		  if(pGameTexture && (pGameTexture->IsEntitySupported() || (texClass == "Normal Bump")))
		  {
        std::string path;
        std::string texName;
        if(pGameTexture->IsEntitySupported())
        {
          path = pGameTexture->GetBitmapFileName();
          texName = pGameTexture->GetTextureName();
        }
        else
        //normal map
        {
          IPropertyContainer* pCont = pGameTexture->GetIPropertyContainer();

          /*
          //prop list
          int numProps = pCont->GetNumberOfProperties();
					for (int propIndex = 0; propIndex < numProps; ++propIndex)
					{
						IGameProperty* pProp = pCont->GetProperty(propIndex);
            std::string pPropName = pProp->GetName();
            EasyOgreExporterLog("Info : texture properties %s\n", pPropName.c_str());
					}
          */

          IGameProperty* pNormalMap = pCont->QueryProperty(_T("normal_map"));
          if(pNormalMap)
          {
            if (pNormalMap->IsParamBlock())
            {
	            if (pNormalMap->IsPBlock2())
	            {
              #ifdef PRE_MAX_2012
                int index = pNormalMap->GetParamBlockIndex();
              #elif PRE_MAX_2011
		            int index = pNormalMap->GetParamBlockIndex();
              #else
                int index = pNormalMap->GetParamIndex();
              #endif

		            IParamBlock2* pBlock = pNormalMap->GetMaxParamBlock2();
		            ParamID id = pBlock->IndextoID(index);
		            Texmap* pTexmap = pBlock->GetTexmap(id);
		            if (pTexmap)
		            {
			            MSTR className;
			            pTexmap->GetClassName(className);
			            const char* pName = className.data();
			            if (!strcmp(pName, "Bitmap"))
			            {
				            BitmapTex* pBitmapTex = static_cast<BitmapTex*>(pTexmap);
                    path = pBitmapTex->GetMapName();
                    texName = pBitmapTex->GetName();
                  }
                }
              }
            }
          }
        }

			  EasyOgreExporterLog("Texture Index: %d\n",i);
        EasyOgreExporterLog("Texture Name: %s\n", texName.c_str());
  		  
		    bool bFoundTexture = false;
        MaxSDK::Util::Path textureName(path.c_str());
			  if(!DoesFileExist(path.c_str()))
			  {
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
            EasyOgreExporterLog("Warning: Couldn't locate texture: %s.\n", texName.c_str());
				  }				
			  }
        else
        {
          bFoundTexture = true;
        }

        Texture tex;
			  tex.absFilename = textureName.GetCStr();
			  std::string filename = textureName.StripToLeaf().GetCStr();
			  tex.filename = optimizeFileName(filename);
        
			  int texSlot = pGameTexture->GetStdMapSlot();
			  switch(texSlot)
			  {
			  case ID_AM:
          {
				    EasyOgreExporterLog("Ambient channel texture.\n");
            if(smat->GetAmbDiffTexLock())
            {
              EasyOgreExporterLog("Ambient channel locked, we use the diffuse texture instead.\n");
              tex.bCreateTextureUnit = false;
            }
            else
            {
              tex.bCreateTextureUnit = bFoundTexture;
              m_hasAmbientMap = true;
              
              if(bFoundTexture)
              {
                BMMRES status;
                BitmapInfo bi(tex.absFilename.c_str());
                Bitmap* bitmap = TheManager->Create(&bi); 
                bitmap = TheManager->Load(&bi, &status); 
                if (status == BMMRES_SUCCESS) 
                  if(bitmap->HasAlpha())
                    m_hasAlpha = true;

                TheManager->DelBitmap(bitmap);
              }
            }
            tex.type = ID_AM;
          }
				  break;
			  case ID_DI:
          {
				    EasyOgreExporterLog("Diffuse channel texture.\n");
				    tex.bCreateTextureUnit = bFoundTexture;
            m_hasDiffuseMap = true;
            
            if(bFoundTexture)
            {
              BMMRES status; 
              BitmapInfo bi(tex.absFilename.c_str());
              Bitmap* bitmap = TheManager->Create(&bi); 
              bitmap = TheManager->Load(&bi, &status); 
              if (status == BMMRES_SUCCESS) 
                if(bitmap->HasAlpha())
                  m_hasAlpha = true;

              TheManager->DelBitmap(bitmap);
            }
            tex.type = ID_DI;
          }
				  break;
			  case ID_SP:
				  EasyOgreExporterLog("Specular channel texture.\n");
          tex.bCreateTextureUnit = bFoundTexture;
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
          tex.bCreateTextureUnit = bFoundTexture;
				  tex.type = ID_SI;
          break;
			  case ID_OP:
				  EasyOgreExporterLog("opacity channel texture.\n");
          //do not create the texture unit
          tex.bCreateTextureUnit = bFoundTexture;
          m_hasAlpha = true;
          tex.type = ID_OP;
				  break;
			  case ID_FI:
				  EasyOgreExporterLog("Filter Color channel texture.\n");
				  tex.type = ID_FI;
          break;
			  case ID_BU:
				  EasyOgreExporterLog("Bump channel texture.\n");
				  tex.bCreateTextureUnit = bFoundTexture;
          m_hasBumpMap = true;
          m_normalMul = smat->GetTexmapAmt(texSlot, 0);
			    tex.type = ID_BU;
          break;
			  case ID_RL:
				  EasyOgreExporterLog("Reflection channel texture.\n");
          tex.type = ID_RL;
          tex.bCreateTextureUnit = bFoundTexture;
          tex.bReflect = true;
          m_reflectivity = smat->GetTexmapAmt(texSlot, 0);
          m_hasReflectionMap = true;
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
			  tex.uvsetName = pGameTexture->GetTextureName();
        tex.uvsetIndex = pGameTexture->GetMapChannel() - 1;
        
        loadTextureUV(pGameTexture, tex);
			  m_textures.push_back(tex);
		  }
	  }
  }

	// Load material data
	bool ExMaterial::load(ParamList& params)
	{
		clear();
		//check if we want to export with lighting off option
		m_lightingOff = params.lightingOff;

		// GET MATERIAL DATA
		if(m_GameMaterial)
		{
      IGameMaterial* pGameMaterial = m_GameMaterial;

      if(m_GameMaterial->IsMultiType() && (m_GameMaterial->GetSubMaterialCount() > 0))
        pGameMaterial = m_GameMaterial->GetSubMaterial(0);
      
			if(!pGameMaterial->IsEntitySupported())
			{
        std::string matClass = m_GameMaterial->GetClassName();

        // Architectural material
        if(matClass == "Architectural")
          loadArchitectureMaterial(pGameMaterial);
        //Mental ray Arch & Design material
        else if(matClass == "Arch & Design")
          loadArchAndDesignMaterial(pGameMaterial);
        else
        {
          EasyOgreExporterLog("Warning: Non supported material : %s... enumerate properties\n", matClass.c_str());
          //enumerate material properties
          MatProc* proccy = new MatProc();
          PropertyEnum* prope = static_cast<PropertyEnum*>(proccy);
          IPropertyContainer* pCont = pGameMaterial->GetIPropertyContainer();
          pCont->EnumerateProperties(*prope);
        }
        
      }
			else
			{
        loadStandardMaterial(pGameMaterial);
			}
		}
		return true;
	}

	bool ExMaterial::exportColor(Point4& color, IGameProperty* pGameProperty)
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
	
  bool ExMaterial::exportSpecular(IGameMaterial* pGameMaterial)
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
  bool ExMaterial::writeOgreScript(ParamList &params, std::ofstream &outMaterial, ExShader* vsAmbShader, ExShader* fpAmbShader, ExShader* vsLightShader, ExShader* fpLightShader)
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
      writeMaterialTechnique(params, outMaterial, -1, vsAmbShader, fpAmbShader, vsLightShader, fpLightShader);
    }

		//End material description
		outMaterial << "}\n";

		//Copy textures to output dir if required
		if (params.copyTextures)
			copyTextures(params);

		return true;
	}

  void ExMaterial::writeMaterialTechnique(ParamList &params, std::ofstream &outMaterial, int lod, ExShader* vsAmbShader, ExShader* fpAmbShader, ExShader* vsLightShader, ExShader* fpLightShader)
  {
		//Start technique description
		outMaterial << "\ttechnique\n";
		outMaterial << "\t{\n";
    if(lod != -1)
      outMaterial << "\t\tlod_index "<< lod <<"\n";
    
    if(vsAmbShader && fpAmbShader)
    {
      writeMaterialPass(params, outMaterial, lod, 1, vsAmbShader, fpAmbShader);
      writeMaterialPass(params, outMaterial, lod, 2, vsLightShader, fpLightShader);
    }
    else
    {
      writeMaterialPass(params, outMaterial, lod, 0, vsLightShader, fpLightShader);
    }

		//End technique description
		outMaterial << "\t}\n";

    // if we got a shader we write a default technique for non supported hardware
    if(vsAmbShader || fpAmbShader || vsLightShader || fpLightShader)
    {
      //Start technique description
	    outMaterial << "\ttechnique\n";
	    outMaterial << "\t{\n";
      if(lod != -1)
        outMaterial << "\t\tlod_index "<< lod <<"\n";
      
      outMaterial << "\tscheme basic_mat\n";
      writeMaterialPass(params, outMaterial, lod, 3, 0, 0);

	    //End technique description
	    outMaterial << "\t}\n";
    }
  }

  void ExMaterial::writeMaterialPass(ParamList &params, std::ofstream &outMaterial, int lod, int mode, ExShader* vsShader, ExShader* fpShader)
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
    
    if(mode == 1)
      outMaterial << "\n\t\t\tillumination_stage ambient\n";
    else if(mode == 2)
    {
      outMaterial << "\n\t\t\tscene_blend add\n";
			outMaterial << "\n\t\t\titeration once_per_light\n";
			outMaterial << "\n\t\t\tillumination_stage per_light\n";
    }

    //if material is transparent set blend mode and turn off depth_writing
		if (m_isTransparent && (mode != 2))
		{
			outMaterial << "\n\t\t\tscene_blend alpha_blend\n";
			outMaterial << "\t\t\tdepth_write off\n";
		}

    if(m_hasAlpha)
			outMaterial << "\n\t\t\talpha_rejection greater 128\n";
		    
    if(vsShader)
      outMaterial << vsShader->getUniformParams(this);

    if(fpShader)
      outMaterial << fpShader->getUniformParams(this);

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
          
          //do not use other textures for ambient pass
          if((mode == 1) && ((m_textures[i].type != ID_AM) && (m_textures[i].type != ID_DI) && (m_textures[i].type != ID_SI) && (m_textures[i].type != ID_RL)))
            continue;

          // do not use this textures for light pass
          if((mode == 2) && ((m_textures[i].type == ID_AM) || (m_textures[i].type == ID_SI)))
            continue;

          // don't export normal and specular maps for non supported technique
          if((mode == 3) && ((m_textures[i].type == ID_BU) || (m_textures[i].type == ID_SP)))
            continue;

				  //start texture unit description
				  outMaterial << "\n\t\t\ttexture_unit\n";
				  outMaterial << "\t\t\t{\n";
				  
          //write texture name
          outMaterial << "\t\t\t\ttexture " << m_textures[i].filename.c_str();
          std::string texExt = m_textures[i].filename.substr(m_textures[i].filename.find_last_of(".") + 1);
				  if((m_textures[i].type == ID_RL) && (texExt == "dds" || texExt == "DDS"))
            outMaterial << " cubic\n";
          else
            outMaterial << "\n";
				  
          //write texture coordinate index
				  outMaterial << "\t\t\t\ttex_coord_set " << m_textures[i].uvsetIndex << "\n";

          //use anisotropic as default for better textures quality
          //outMaterial << "\t\t\t\tfiltering anisotropic\n";

          if((m_textures[i].fAmount >= 1.0f) || (m_textures[i].type == ID_BU))
          {
            outMaterial << "\t\t\t\tcolour_op modulate\n";
          }
          else
          {
            outMaterial << "\t\t\t\tcolour_op_ex blend_manual src_texture src_current " << (((mode == 3) && (m_textures[i].type == ID_RL)) ? m_textures[i].fAmount / 3.0f : m_textures[i].fAmount) << "\n";
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
	bool ExMaterial::copyTextures(ParamList &params)
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
