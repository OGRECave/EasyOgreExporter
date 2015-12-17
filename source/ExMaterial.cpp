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
#include "ExOgreConverter.h"
#include "ExTools.h"
#include "EasyOgreExporterLog.h"
#include <imtl.h>

#include <nvimage/DirectDrawSurface.h>

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
    #ifdef UNICODE
      EasyOgreExporterLog("Debug: Material properties : %ls\n", prop->GetName());
    #else
      EasyOgreExporterLog("Debug: Material properties : %s\n", prop->GetName());
    #endif
		return false;//lets keep going
	}


	// Constructor
	ExMaterial::ExMaterial(ExOgreConverter* converter, IGameMaterial* pGameMaterial, std::string prefix)
	{
    m_converter = converter;
		m_GameMaterial = pGameMaterial;
		m_name = getMaterialName(prefix);
		clear();
	}

	// Destructor
	ExMaterial::~ExMaterial()
	{
		m_textures.clear();
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
    m_softness = 0.1f;
		m_reflectivity = 0.0f;
		m_normalMul = 1.0f;
		m_isTwoSided = false;
		m_isWire = false;
		m_hasAlpha = false;
		m_bPreMultipliedAlpha = true;
		m_hasAmbientMap = false;
		m_ambientLocked = false;
		m_hasDiffuseMap = false;
		m_hasSpecularMap = false;
		m_hasReflectionMap = false;
		m_hasBumpMap = false;
    m_AmbIsVertColor = false;
    m_DiffIsVertColor = false;

		texUnitId = 0;
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

#ifdef UNICODE
		std::wstring maxmat_w = m_GameMaterial->GetMaterialName();
		std::string maxmat;
		maxmat.assign(maxmat_w.begin(), maxmat_w.end());
#else
		std::string maxmat = m_GameMaterial->GetMaterialName();
#endif

		trim(maxmat);
		tmpStr.append(maxmat);

		newMatName = optimizeResourceName(tmpStr);
		return newMatName;
	}

	std::string ExMaterial::getShaderName(ExShader::ShaderType type, std::string prefix)
	{
		std::string sname;
		std::stringstream out;

		out << prefix;
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
              if (m_type != MT_METAL)
                out << "Fresnel";
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
							out << "EMI";
							out << m_textures[i].uvsetIndex;
							break;

						case ID_RL:
              if (m_type != MT_METAL)
                out << "Fresnel";
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
            bool isAnimatedUv = ((m_textures[i].scroll_s_u != 0.0) || (m_textures[i].scroll_s_v != 0.0) || (m_textures[i].rot_s != 0.0)) ? true : false;

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

            case ID_SI:
              texUnits.push_back(m_textures[i].uvsetIndex);
              break;

						case ID_RL:
							break;
						}

            if (isAnimatedUv)
              out << "A";
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

        bool isMask = false;
				for (int i=0; i<m_textures.size(); i++)
				{
					if(m_textures[i].bCreateTextureUnit == true)
					{
						switch (m_textures[i].type)
						{
						case ID_AM:
              if (!isMask)
							  out << "AMB";
              else
                out << "MSK";
							out << m_textures[i].uvsetIndex;
							break;

						case ID_DI:
              if (!isMask)
                out << "DIFF";
              else
                out << "MSK";
							out << m_textures[i].uvsetIndex;
							break;

						case ID_SP:
              if (!isMask)
                out << "SPEC";
              else
                out << "MSK";
							out << m_textures[i].uvsetIndex;
							break;

            case ID_SI:
              if (!isMask)
                out << "EMI";
              else
                out << "MSK";
              out << m_textures[i].uvsetIndex;
              break;

						case ID_BU:
              if (!isMask)
                out << "NORM";
              else
                out << "MSK";
							out << m_textures[i].uvsetIndex;
							break;

						case ID_RL:
              if (!isMask)
              {
                if (m_type != MT_METAL)
                  out << "Fresnel";
                out << "REF";
              }
              else
                out << "MSK";

              break;
						}

            isMask = m_textures[i].hasMask;
					}
				}
				break;
			}

		case ExShader::ST_VSLIGHT_MULTI :
			{
				out << "vsLightMultiGEN";

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

		case ExShader::ST_FPLIGHT_MULTI:
			{
				out << "fpLightMultiGEN";

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
              if (m_type != MT_METAL)
                out << "Fresnel";
							out << "REF";
							break;
						}
					}
				}
				break;
			}
		}

		sname = optimizeResourceName(out.str());
		return sname;
	}

	void ExMaterial::loadTextureUV(IGameTextureMap* pGameTexture, Texture &tex)
	{
		if(!pGameTexture || !pGameTexture->IsEntitySupported())
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

        if (prop->IsPropAnimated())
        {
          float aRot = 0.0f;
          prop->GetPropertyValue(aRot, GetCOREInterface()->GetAnimRange().End());
          tex.rot_s = (aRot - rot) / static_cast<float>((GetCOREInterface()->GetAnimRange().End() - GetCOREInterface()->GetAnimRange().Start()) / static_cast<float>(GetTicksPerFrame()) / GetFrameRate());
        }
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

        if (prop->IsPropAnimated())
        {
          float aTransU = 0.0f;
          prop->GetPropertyValue(aTransU, GetCOREInterface()->GetAnimRange().End());
          tex.scroll_s_u = (aTransU - transU) / static_cast<float>((GetCOREInterface()->GetAnimRange().End() - GetCOREInterface()->GetAnimRange().Start()) / static_cast<float>(GetTicksPerFrame()) / GetFrameRate());
        }
			}
			prop = pUVGen->GetVOffsetData();
			if(prop)
			{
				prop->GetPropertyValue(transV);
				tex.scroll_v = 0.5 * (covV-1.0)/covV + transV/covV;
				if (fabs(tex.scroll_v) < PRECISION)
					tex.scroll_v = 0;

        if (prop->IsPropAnimated())
        {
          float aTransV = 0.0f;
          prop->GetPropertyValue(aTransV, GetCOREInterface()->GetAnimRange().End());
          tex.scroll_s_v = (aTransV - transV) / static_cast<float>((GetCOREInterface()->GetAnimRange().End() - GetCOREInterface()->GetAnimRange().Start()) / static_cast<float>(GetTicksPerFrame()) / GetFrameRate());
        }
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
					ParamID id = pBlock->IndextoID(index);
					Texmap* pTexmap = pBlock->GetTexmap(id);
					if (pTexmap)
					{
						MSTR className;
						pTexmap->GetClassName(className);
#ifdef UNICODE
						std::wstring pName_w = className.data();
						std::string pName_s;
						pName_s.assign(pName_w.begin(), pName_w.end());
						const char* pName = pName_s.data();
#else
						const char* pName = className.data();
#endif
            EasyOgreExporterLog("Info : Texture class name: %s.\n", pName);
            if (!strcmp(pName, "Vertex Color"))
            {
              if (type == ID_DI)
                m_DiffIsVertColor = true;
              else if (type == ID_AM)
                m_AmbIsVertColor = true;
            }
						else if (!strcmp(pName, "Bitmap"))
						{
							BitmapTex* pBitmapTex = static_cast<BitmapTex*>(pTexmap);

#ifdef UNICODE
              MSTR texName = pBitmapTex->GetMapName();
              EasyOgreExporterLog("Texture Name: %ls\n", texName.data());
#else
              std::string texName = pBitmapTex->GetMapName();
              EasyOgreExporterLog("Texture Name: %s\n", texName.c_str());
#endif

              //absolute texture path
              std::string absPath;
							bool bFoundTexture = GetMaxFilePath(texName, absPath);

							if(bFoundTexture == true)
							{
                EasyOgreExporterLog("Updated texture location: %s.\n", absPath.c_str());
							}
							else
							{
#ifdef UNICODE
					      EasyOgreExporterLog("Warning: Couldn't locate texture: %ls.\n", texName.data());
#else
					      EasyOgreExporterLog("Warning: Couldn't locate texture: %s.\n", texName.c_str());
#endif
							}

							Texture tex;
							tex.bCreateTextureUnit = true;
							tex.uvsetIndex = 0;
							tex.uvsetIndex = pBitmapTex->GetMapChannel() - 1;

							tex.absFilename.push_back(absPath);
              tex.filename.push_back(optimizeFileName(m_converter->getMaterialSet()->getUniqueTextureName(absPath)));
							tex.fAmount = amount;

							if(bFoundTexture)
							{
								BMMRES status;
#ifdef UNICODE
								std::string absFilename_s = tex.absFilename[0].c_str();
								std::wstring absFilename_w;
								absFilename_w.assign(absFilename_s.begin(),absFilename_s.end());
								BitmapInfo bi(absFilename_w.c_str());
#else
								BitmapInfo bi(tex.absFilename[0].c_str());
#endif
								//Bitmap* bitmap = TheManager->Create(&bi);
								Bitmap* bitmap = TheManager->Load(&bi, &status);
								if (status == BMMRES_SUCCESS)
                {
									if(bitmap->HasAlpha())
									{
										m_hasAlpha = (pBitmapTex->GetAlphaSource() == ALPHA_NONE) ? false : true;
										m_bPreMultipliedAlpha = pBitmapTex->GetPremultAlpha(0) ? true : false;
									}

									TheManager->DelBitmap(bitmap);
                  bitmap->DeleteThis();
                }

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
			m_specular.w = 1.0;
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
			m_specular.w = 1.0;
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

		m_ambientLocked = smat->GetAmbDiffTexLock() ? true : false;
		m_isTwoSided = smat->GetTwoSided() ? true : false;
		m_isWire = smat->GetWire() ? true : false;
		if(smat->IsFaceted())
		{
			m_type = MT_FACETED;
		}
		else
		{
      if (smat->GetShading() == SHADE_PHONG)
        m_type = MT_PHONG;
      else if (smat->GetShading() == SHADE_METAL)
        m_type = MT_METAL;
		}

		if(!maxMat->GetSelfIllumColorOn())
		{
			IGameProperty* pGameProperty = pGameMaterial->GetEmissiveAmtData();
			if (pGameProperty && IGAME_FLOAT_PROP == pGameProperty->GetType())
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
      int tmpTexChannel = -1;
      int tmpMaskTexChannel = -1;
			IGameTextureMap* pGameTexture = pGameMaterial->GetIGameTextureMap(i);
      int texSlot = pGameTexture->GetStdMapSlot();

      //skip disabled texmap
      if (texSlot < 0 || maxMat->SubTexmapOn(texSlot) == 0)
        continue;
      
#ifdef UNICODE
			std::wstring texClass = pGameTexture->GetClassName();
#else
			std::string texClass = pGameTexture->GetClassName();
#endif
			IPropertyContainer* pCont = pGameTexture->GetIPropertyContainer();

#ifdef UNICODE
      EasyOgreExporterLog("Exporting %d texture from %ls...\n", i, texClass.data());
      if (pGameTexture && (pGameTexture->IsEntitySupported() || (texClass == L"Composite") || (texClass == L"Normal Bump")) || (texClass == L"Vertex Color"))
#else
      EasyOgreExporterLog("Exporting %d texture from %s...\n", i, texClass.c_str());
      if(pGameTexture && (pGameTexture->IsEntitySupported() || (texClass == "Composite") || (texClass == "Normal Bump")) || (texClass == "Vertex Color"))
#endif
      {
#ifdef UNICODE
        MSTR path;
        MSTR texName;
        MSTR maskPath;
        MSTR maskTexName;
#else
        std::string path;
        std::string texName;
        std::string maskPath;
        std::string maskTexName;
#endif
        if (pGameTexture->IsEntitySupported())
        {
          texName = pGameTexture->GetTextureName();

          // hack in case of the user cancel the bitmap add, max sdk just crash on null pointer ...
#ifdef UNICODE
          if (!texName.isNull())
#else
          if(!texName.empty())
#endif
            path = pGameTexture->GetBitmapFileName();
        }
        else
#ifdef UNICODE
        if ((texClass == L"Normal Bump") || (texClass == L"Vertex Color"))
#else
        if ((texClass == "Normal Bump") || (texClass == "Vertex Color"))
#endif
          //normal map
        {
          IGameProperty* pNormalMap = pCont->QueryProperty(_T("normal_map"));
          if (pNormalMap)
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
#ifdef UNICODE
                  const wchar_t* pName = className.data();
                  if (!wcscmp(pName, L"Bitmap"))
#else
                  const char* pName = className.data();
                  if (!strcmp(pName, "Bitmap"))
#endif
                  {
                    BitmapTex* pBitmapTex = static_cast<BitmapTex*>(pTexmap);
                    path = pBitmapTex->GetMapName();
                    texName = pBitmapTex->GetName();
                    tmpTexChannel = pBitmapTex->GetMapChannel();
                  }
                }
              }
            }
          }
        }
        else
#ifdef UNICODE
        if ((texClass == L"Composite"))
#else
        if ((texClass == "Composite"))
#endif
        {
          IGameProperty* pMapList = pCont->QueryProperty(_T("mapList"));
          if (pMapList)
          {
            if (pMapList->IsParamBlock())
            {
              if (pMapList->IsPBlock2())
              {
#ifdef PRE_MAX_2012
                int index = pMapList->GetParamBlockIndex();
#elif PRE_MAX_2011
                int index = pMapList->GetParamBlockIndex();
#else
                int index = pMapList->GetParamIndex();
#endif

                IParamBlock2* pBlock = pMapList->GetMaxParamBlock2();
                ParamID id = pBlock->IndextoID(index);
                Texmap* pTexmap = pBlock->GetTexmap(id);
                if (pTexmap)
                {
                  MSTR className;
                  pTexmap->GetClassName(className);
#ifdef UNICODE
                  const wchar_t* pName = className.data();
                  if (!wcscmp(pName, L"Bitmap"))
#else
                  const char* pName = className.data();
                  if (!strcmp(pName, "Bitmap"))
#endif
                  {
                    BitmapTex* pBitmapTex = static_cast<BitmapTex*>(pTexmap);
                    path = pBitmapTex->GetMapName();
                    texName = pBitmapTex->GetName();
                    tmpTexChannel = pBitmapTex->GetMapChannel();
                  }
                }

                //Get mask
                IGameProperty* pMaskEnable = pCont->QueryProperty(_T("maskEnabled"));
                int maskEnable = 0;
                if (pMaskEnable)
                {
                  if (pMaskEnable->IsParamBlock())
                  {
                    if (pMaskEnable->IsPBlock2())
                    {
#ifdef PRE_MAX_2012
                      int index = pMaskEnable->GetParamBlockIndex();
#elif PRE_MAX_2011
                      int index = pMaskEnable->GetParamBlockIndex();
#else
                      int index = pMaskEnable->GetParamIndex();
#endif

                      IParamBlock2* pBlock = pMaskEnable->GetMaxParamBlock2();
                      ParamID id = pBlock->IndextoID(index);
                      maskEnable = pBlock->GetInt(id);
                    }
                  }
                }

                if (maskEnable)
                {
                  IGameProperty* pMask = pCont->QueryProperty(_T("mask"));
                  if (pMask)
                  {
                    if (pMask->IsParamBlock())
                    {
                      if (pMask->IsPBlock2())
                      {
#ifdef PRE_MAX_2012
                        int index = pMask->GetParamBlockIndex();
#elif PRE_MAX_2011
                        int index = pMask->GetParamBlockIndex();
#else
                        int index = pMask->GetParamIndex();
#endif

                        IParamBlock2* pBlock = pMask->GetMaxParamBlock2();
                        ParamID id = pBlock->IndextoID(index);
                        Texmap* pTexmap = pBlock->GetTexmap(id);
                        if (pTexmap)
                        {
                          MSTR className;
                          pTexmap->GetClassName(className);
#ifdef UNICODE
                          const wchar_t* pName = className.data();
                          if (!wcscmp(pName, L"Bitmap"))
#else
                          const char* pName = className.data();
                          if (!strcmp(pName, "Bitmap"))
#endif
                          {
                            BitmapTex* pBitmapTex = static_cast<BitmapTex*>(pTexmap);
                            maskPath = pBitmapTex->GetMapName();
                            maskTexName = pBitmapTex->GetName();
                            tmpMaskTexChannel = pBitmapTex->GetMapChannel();
                          }
                        }
                      }
                    }
                  }

                }
              }
            }
          }
        }
        else
        {
#ifdef UNICODE
          EasyOgreExporterLog("Warning: Non supported texture : %ls... enumerate properties\n", texClass.data());
#else
          EasyOgreExporterLog("Warning: Non supported texture : %s... enumerate properties\n", texClass.c_str());
#endif
          //enumerate material properties
          MatProc* proccy = new MatProc();
          PropertyEnum* prope = static_cast<PropertyEnum*>(proccy);
          IPropertyContainer* pCont = pGameTexture->GetIPropertyContainer();
          pCont->EnumerateProperties(*prope);
        }

        //absolute texture path
        std::string absPath;
        std::string absMaskPath;

        bool bFoundTextureMask = false;

        EasyOgreExporterLog("Texture Index: %d\n", i);
#ifdef UNICODE
        EasyOgreExporterLog("Texture Name: %ls\n", texName.data());
        if (maskTexName.data())
        {
          bFoundTextureMask = true;
          EasyOgreExporterLog("Texture Name for Mask: %ls\n", maskTexName.data());
        }
#else
        EasyOgreExporterLog("Texture Name: %s\n", texName.c_str());
        if (!maskTexName.empty())
        {
          bFoundTextureMask = true;
          EasyOgreExporterLog("Texture Name for Mask: %s\n", maskTexName.c_str());
        }
#endif
        bool bFoundTexture = GetMaxFilePath(path, absPath);
				
        if(bFoundTexture == true)
				{
          EasyOgreExporterLog("Updated texture location: %s.\n", absPath.c_str());
				}
				else
				{
#ifdef UNICODE
					EasyOgreExporterLog("Warning: Couldn't locate texture: %ls.\n", texName.data());
#else
					EasyOgreExporterLog("Warning: Couldn't locate texture: %s.\n", texName.c_str());
#endif
				}

				Texture tex;
        {
          // set texture paths
          std::string abslower = absPath;
          std::transform(abslower.begin(), abslower.end(), abslower.begin(), ::tolower);
          if (FileExt(abslower) != ".ifl")
          {
            tex.absFilename.push_back(absPath);
            tex.filename.push_back(optimizeFileName(m_converter->getMaterialSet()->getUniqueTextureName(absPath)));
          }
          else
          {
            bFoundTexture = false;
            int animRate = 1;
            std::vector<std::string> iflPaths = ReadIFL(absPath, animRate);
            for (unsigned int k = 0; k < iflPaths.size(); k++)
            {
              std::string gpath;
              bFoundTexture = GetMaxFilePath(iflPaths[k], gpath);
              tex.absFilename.push_back(gpath);
              tex.filename.push_back(optimizeFileName(m_converter->getMaterialSet()->getUniqueTextureName(gpath)));
            }
            tex.animRate = animRate;
          }
        }

        //get the texture multiplier
        tex.fAmount = smat->GetTexmapAmt(texSlot, 0);

        if (bFoundTexture)
        {
          BMMRES status;
#ifdef UNICODE
          std::string absFilename_s = tex.absFilename[0].c_str();
          std::wstring absFilename_w;
          absFilename_w.assign(absFilename_s.begin(), absFilename_s.end());
          BitmapInfo bi(absFilename_w.c_str());
#else
          BitmapInfo bi(tex.absFilename[0].c_str());
#endif

          Bitmap* bitmap = TheManager->Load(&bi, &status);
          if (status == BMMRES_SUCCESS)
          {
            tex.bHasAlphaChannel = (bitmap->HasAlpha() == 0) ? false : true;

            TheManager->DelBitmap(bitmap);
            bitmap->DeleteThis();
          }
        }

        if (bFoundTextureMask)
        {
          bFoundTextureMask = GetMaxFilePath(maskPath, absMaskPath);
          EasyOgreExporterLog("Updated texture Mask location: %s.\n", absMaskPath.c_str());
        }

        Texture texMask;
        if (bFoundTextureMask)
        {
          // set texture paths
          std::string abslower = absMaskPath;
          std::transform(abslower.begin(), abslower.end(), abslower.begin(), ::tolower);
          if (FileExt(abslower) != ".ifl")
          {
            texMask.absFilename.push_back(absMaskPath);
            texMask.filename.push_back(optimizeFileName(m_converter->getMaterialSet()->getUniqueTextureName(absMaskPath)));
          }
          else
          {
            bFoundTextureMask = false;
            int animRate = 1;
            std::vector<std::string> iflPaths = ReadIFL(absMaskPath, animRate);
            for (unsigned int k = 0; k < iflPaths.size(); k++)
            {
              std::string gpath;
              bFoundTextureMask = GetMaxFilePath(iflPaths[k], gpath);
              texMask.absFilename.push_back(gpath);
              texMask.filename.push_back(optimizeFileName(m_converter->getMaterialSet()->getUniqueTextureName(gpath)));
            }
            texMask.animRate = animRate;
          }
        }

        if (bFoundTextureMask)
        {
          BMMRES status;
#ifdef UNICODE
          std::string absFilename_s = texMask.absFilename[0].c_str();
          std::wstring absFilename_w;
          absFilename_w.assign(absFilename_s.begin(), absFilename_s.end());
          BitmapInfo bi(absFilename_w.c_str());
#else
          BitmapInfo bi(texMask.absFilename[0].c_str());
#endif

          Bitmap* bitmap = TheManager->Load(&bi, &status);
          if (status == BMMRES_SUCCESS)
          {
            texMask.bHasAlphaChannel = (bitmap->HasAlpha() == 0) ? false : true;

            TheManager->DelBitmap(bitmap);
            bitmap->DeleteThis();
          }
        }

				switch(texSlot)
				{
				case ID_AM:
					{
						EasyOgreExporterLog("Ambient channel texture.\n");
						if(m_ambientLocked)
						{
							EasyOgreExporterLog("Ambient channel locked, we use the diffuse texture instead.\n");
							tex.bCreateTextureUnit = false;
              texMask.bCreateTextureUnit = false;
						}
						else
						{
							tex.bCreateTextureUnit = bFoundTexture;
              texMask.bCreateTextureUnit = bFoundTextureMask;
							if(bFoundTexture)
							{
                m_hasAmbientMap = bFoundTexture;
							}
#ifdef UNICODE
			        else if (texClass == L"Vertex Color")
#else
              else if (texClass == "Vertex Color")
#endif
              {
                m_AmbIsVertColor = true;
              }
						}

						tex.type = ID_AM;
            texMask.type = ID_AM;
					}
					break;
				case ID_DI:
					{
						EasyOgreExporterLog("Diffuse channel texture.\n");
						tex.bCreateTextureUnit = bFoundTexture;
            texMask.bCreateTextureUnit = bFoundTextureMask;

						if(bFoundTexture)
						{
              if(tex.bHasAlphaChannel)
              {
                int alphaSrc = 0;
							  IGameProperty* pAlphaSrc = pCont->QueryProperty(_T("alphaSource"));
							  if(pAlphaSrc)
							  {
								  pAlphaSrc->GetPropertyValue(alphaSrc);
								  m_hasAlpha = ((alphaSrc + 1) == ALPHA_NONE) ? false : true;
							  }
              }

							int preMult = 0;
							IGameProperty* pPreMult = pCont->QueryProperty(_T("preMultAlpha"));
							if(pPreMult)
							{
								pPreMult->GetPropertyValue(preMult);
								m_bPreMultipliedAlpha = preMult ? true : false;
							}

              m_hasDiffuseMap = true;
						}
#ifdef UNICODE
			      else if (texClass == L"Vertex Color")
#else
            else if (texClass == "Vertex Color")
#endif
            {
              m_DiffIsVertColor = true;
            }

						tex.fAmount *= m_opacity;
						tex.type = ID_DI;
            texMask.type = ID_DI;
					}
					break;
				case ID_SP:
					EasyOgreExporterLog("Specular channel texture.\n");
					tex.bCreateTextureUnit = bFoundTexture;
          texMask.bCreateTextureUnit = bFoundTextureMask;
					m_hasSpecularMap = true;
					tex.type = ID_SP;
          texMask.type = ID_SP;
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
          texMask.bCreateTextureUnit = bFoundTextureMask;
					tex.type = ID_SI;
          texMask.type = ID_SI;
					break;
				case ID_OP:
					EasyOgreExporterLog("opacity channel texture.\n");
					tex.bCreateTextureUnit = bFoundTexture;
          texMask.bCreateTextureUnit = bFoundTextureMask;

          //only use the texture opacity properties, the alpha channel should be in the diffuse texture
          if(bFoundTexture)
					{
            if(tex.bHasAlphaChannel)
            {
              int alphaSrc = 0;
						  IGameProperty* pAlphaSrc = pCont->QueryProperty(_T("alphaSource"));
						  if(pAlphaSrc)
						  {
							  pAlphaSrc->GetPropertyValue(alphaSrc);
							  m_hasAlpha = ((alphaSrc + 1) == ALPHA_NONE) ? m_hasAlpha : true;
						  }
            }

						int preMult = 0;
						IGameProperty* pPreMult = pCont->QueryProperty(_T("preMultAlpha"));
						if(pPreMult)
						{
							pPreMult->GetPropertyValue(preMult);
							m_bPreMultipliedAlpha = preMult ? true : m_bPreMultipliedAlpha;
						}
					}

					tex.type = ID_OP;
          texMask.type = ID_OP;
					break;
				case ID_FI:
					EasyOgreExporterLog("Filter Color channel texture.\n");
					tex.type = ID_FI;
					break;
				case ID_BU:
					EasyOgreExporterLog("Bump channel texture.\n");
					tex.bCreateTextureUnit = bFoundTexture;
          texMask.bCreateTextureUnit = bFoundTextureMask;

					m_hasBumpMap = bFoundTexture;
					m_normalMul = smat->GetTexmapAmt(texSlot, 0);
					tex.type = ID_BU;
          texMask.type = ID_BU;
					break;
				case ID_RL:
					EasyOgreExporterLog("Reflection channel texture.\n");
          tex.bCreateTextureUnit = bFoundTexture;
          tex.type = ID_RL;
					tex.bReflect = bFoundTexture;

          texMask.bCreateTextureUnit = bFoundTextureMask;
          texMask.type = ID_RL;
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

        tex.hasMask = texMask.bCreateTextureUnit;
        tex.uvsetIndex = (tmpTexChannel >= 1) ? tmpTexChannel -1 : pGameTexture->GetMapChannel() - 1;
				loadTextureUV(pGameTexture, tex);
				m_textures.push_back(tex);

        texMask.uvsetIndex = (tmpMaskTexChannel >= 1) ? tmpMaskTexChannel - 1 : pGameTexture->GetMapChannel() - 1;
        if (texMask.bCreateTextureUnit)
        {
          loadTextureUV(pGameTexture, texMask);
          m_textures.push_back(texMask);
        }
			}
      else
      {
        //prop list
        int numProps = pCont->GetNumberOfProperties();
        for (int propIndex = 0; propIndex < numProps; ++propIndex)
        {
#ifdef UNICODE
          EasyOgreExporterLog("Info : not managed texture properties : %ls\n", texClass.data());
#else
          EasyOgreExporterLog("Info : not managed texture properties : %s\n", texClass.c_str());
#endif
          /*
          IGameProperty* pProp = pCont->GetProperty(propIndex);
          if (pProp)
          {
#ifdef UNICODE
            EasyOgreExporterLog("Info : not managed texture properties : %ls\n", pProp->GetName());
#else
            EasyOgreExporterLog("Info : not managed texture properties : %s\n", pProp->GetName());
#endif
          }
          */
        }
      }
		}
	}

	// Load material data
	bool ExMaterial::load()
	{
    ParamList params = m_converter->getParams();
		clear();
		//check if we want to export with lighting off option
		m_lightingOff = params.lightingOff;

		// GET MATERIAL DATA
		if(m_GameMaterial)
		{
			IGameMaterial* pGameMaterial = m_GameMaterial;

      //TODO manage mix / composite materials
			if(m_GameMaterial->IsMultiType() && (m_GameMaterial->GetSubMaterialCount() > 0))
				pGameMaterial = m_GameMaterial->GetSubMaterial(0);

			if(!pGameMaterial->IsEntitySupported())
			{
#ifdef UNICODE
				std::wstring matClass_w = m_GameMaterial->GetClassName();
				std::string matClass;
				matClass.assign(matClass_w.begin(),matClass_w.end());
#else
				std::string matClass = m_GameMaterial->GetClassName();
#endif

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
		if(pGameProperty && IGAME_POINT4_PROP == pGameProperty->GetType())
		{
			Point4 matColor;
			if(pGameProperty->GetPropertyValue(matColor))
			{
				color = matColor;
				return true;
			}
		}
		else if (pGameProperty && IGAME_POINT3_PROP == pGameProperty->GetType())
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
		if(pGameProperty && IGAME_POINT4_PROP == pGameProperty->GetType())
		{
			Point4 matColor;
			if(pGameProperty->GetPropertyValue(matColor))
			{
				m_specular = matColor;
			}
		}
		else if (pGameProperty && IGAME_POINT3_PROP == pGameProperty->GetType())
		{
			Point3 color3;
			if(pGameProperty->GetPropertyValue(color3))
			{
				m_specular.x = color3.x;
				m_specular.y = color3.y;
				m_specular.z = color3.z;
				m_specular.w = 1.0; 
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

    IPropertyContainer* pCont = pGameMaterial->GetIPropertyContainer();
    float softness = 0.0f;
    IGameProperty* pSoften = pCont->QueryProperty(_T("soften"));
    if (pSoften)
    {
      pSoften->GetPropertyValue(softness);
      m_softness = softness;
    }

		return true;
	}

	// Write material data to an Ogre material script file
	bool ExMaterial::writeOgreScript(std::ofstream &outMaterial, ExShader* vsAmbShader, ExShader* fpAmbShader, ExShader* vsLightShader, ExShader* fpLightShader)
	{
    ParamList params = m_converter->getParams();

		//Start material description
		outMaterial << "material \"" << m_name.c_str() << "\"\n";
		outMaterial << "{\n";

    if (params.generateLOD)
		{
		  outMaterial << "\tlod_strategy screen_ratio_pixel_count\n";
      outMaterial << "\tlod_values 0.1\n";
      writeMaterialTechnique(outMaterial, 0, vsAmbShader, fpAmbShader, vsLightShader, fpLightShader);
		}
		else
		{
			writeMaterialTechnique(outMaterial, -1, vsAmbShader, fpAmbShader, vsLightShader, fpLightShader);
		}

		//End material description
		outMaterial << "}\n";

		return true;
	}

	void ExMaterial::writeMaterialTechnique(std::ofstream &outMaterial, int lod, ExShader* vsAmbShader, ExShader* fpAmbShader, ExShader* vsLightShader, ExShader* fpLightShader)
	{
		//Start technique description
		outMaterial << "\ttechnique ";
		outMaterial << m_name << "_technique" << "\n";
		outMaterial << "\t{\n";
		if(lod != -1)
			outMaterial << "\t\tlod_index "<< lod <<"\n";

		if(vsAmbShader || fpAmbShader)
		{
      if (vsAmbShader)
      {
        writeMaterialPass(outMaterial, lod, vsAmbShader, fpAmbShader, ExShader::SP_AMBIENT);
        writeMaterialPass(outMaterial, lod, vsLightShader, fpLightShader, ExShader::SP_LIGHT_MULTI);
      }
      else
      {
        writeMaterialPass(outMaterial, lod, vsLightShader, fpLightShader, ExShader::SP_LIGHT);
      }
		}
		else
		{
			writeMaterialPass(outMaterial, lod, vsLightShader, fpLightShader, ExShader::SP_NONE);
		}

		//End technique description
		outMaterial << "\t}\n";

    // if we got a shader we write a default technique without shader
    if (vsAmbShader || fpAmbShader || vsLightShader || fpLightShader)
		{
			//Start technique description
			outMaterial << "\ttechnique ";
			outMaterial << m_name << "_basic_technique" << "\n";
			outMaterial << "\t{\n";
      outMaterial << "\tscheme basic_mat\n";

			writeMaterialPass(outMaterial, lod, 0, 0, ExShader::SP_NOSUPPORT);

			//End technique description
			outMaterial << "\t}\n";
		}

    // if we got a shader we write a LOD technique without scheme
    if ((lod != -1) && (vsAmbShader || fpAmbShader || vsLightShader || fpLightShader))
    {
      //Start technique description
      outMaterial << "\ttechnique ";
      outMaterial << m_name << "_lod_technique" << "\n";
      outMaterial << "\t{\n";
      if (lod != -1)
        outMaterial << "\t\tlod_index " << lod + 1 << "\n";

      writeMaterialPass(outMaterial, lod, 0, 0, ExShader::SP_NOSUPPORT);

      //End technique description
      outMaterial << "\t}\n";
    }
	}

	void ExMaterial::writeMaterialPass(std::ofstream &outMaterial, int lod, ExShader* vsShader, ExShader* fpShader, ExShader::ShaderPass pass)
	{
    ParamList params = m_converter->getParams();

		//Start render pass description
		outMaterial << "\t\tpass ";
		outMaterial << m_name << "_";
		if(pass == ExShader::SP_AMBIENT)
			outMaterial << "Ambient\n";
		else if(pass == ExShader::SP_LIGHT)
			outMaterial << "Light\n";
    else if (pass == ExShader::SP_LIGHT_MULTI)
      outMaterial << "LightMulti\n";
		else if(pass == ExShader::SP_DECAL)
			outMaterial << "Decal\n";
		else
			outMaterial << "standard\n";

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
		if((m_hasAmbientMap || (m_hasDiffuseMap && m_ambientLocked)) && (lod <= 0))
		{
			m_ambient.x = 1.0f;
			m_ambient.y = 1.0f;
			m_ambient.z = 1.0f;
		}
    if(m_AmbIsVertColor || (m_ambientLocked && m_DiffIsVertColor))
		  outMaterial << "\t\t\tambient vertexcolour\n";
    else
      outMaterial << "\t\t\tambient " << m_ambient.x << " " << m_ambient.y << " " << m_ambient.z << " " << m_ambient.w << "\n";

		//diffuse colour
		//Format: diffuse (<red> <green> <blue> [<alpha>]| vertexcolour)
		if(m_hasDiffuseMap && (lod < 2))
		{
			m_diffuse.x = 1.0f;
			m_diffuse.y = 1.0f;
			m_diffuse.z = 1.0f;
		}

    if(m_DiffIsVertColor)
		  outMaterial << "\t\t\tdiffuse vertexcolour\n";
    else
		  outMaterial << "\t\t\tdiffuse " << m_diffuse.x << " " << m_diffuse.y << " " << m_diffuse.z << " " << m_diffuse.w << "\n";

		//specular colour and shininess
		if(m_hasSpecularMap && (lod <= 0))
		{
			m_specular.x = 1.0f;
			m_specular.y = 1.0f;
			m_specular.z = 1.0f;
		}
		outMaterial << "\t\t\tspecular " << m_specular.x << " " << m_specular.y << " " << m_specular.z	<< " " << m_specular.w << " " << m_shininess << "\n";

		//emissive colour
		outMaterial << "\t\t\temissive " << m_emissive.x << " " << m_emissive.y << " " << m_emissive.z << " " << m_emissive.w << "\n";

		if(pass == ExShader::SP_AMBIENT)
			outMaterial << "\n\t\t\tillumination_stage ambient\n";
		else if(pass == ExShader::SP_LIGHT_MULTI)
		{
			if(m_hasAlpha)
        outMaterial << "\n\t\t\tseparate_scene_blend src_alpha one src_alpha one_minus_src_alpha \n";
        //outMaterial << "\n\t\t\tseparate_scene_blend add modulate\n";
			else
				outMaterial << "\n\t\t\tscene_blend add\n";

			outMaterial << "\n\t\t\titeration once_per_light\n";
			outMaterial << "\n\t\t\tillumination_stage per_light\n";
			outMaterial << "\t\t\tdepth_write off\n";
		}

		//if material is transparent set blend mode and turn off depth_writing
		if (m_isTransparent || (m_bPreMultipliedAlpha && m_hasAlpha) && ((pass == ExShader::SP_AMBIENT) || (pass == ExShader::SP_NONE) || (pass == ExShader::SP_NOSUPPORT)))
		{
			outMaterial << "\n\t\t\tscene_blend alpha_blend\n";
			outMaterial << "\t\t\tdepth_write off\n";
		}

		if(m_hasAlpha && !m_bPreMultipliedAlpha)
			outMaterial << "\n\t\t\talpha_rejection greater 128\n";

		if(vsShader)
			outMaterial << vsShader->getUniformParams(this);

		if(fpShader)
			outMaterial << fpShader->getUniformParams(this);

		//write texture units
		//TODO manage ambient, diffuse, spec, alpha layer types with colour_op_ex <operation> <source1> <source2> [<manual_factor>] [<manual_colour1>] [<manual_colour2>]
		if(lod < 2) // no texture for last LOD
		{
      bool isMask = false;
			for (int i=0; i<m_textures.size(); i++)
			{
				if(m_textures[i].bCreateTextureUnit == true)
				{
					//only diffuse on next lod
					if((m_textures[i].type != ID_DI) && (lod > 0))
						continue;

					//do not use other textures for ambient pass
					if((pass == ExShader::SP_AMBIENT) && ((m_textures[i].type != ID_AM) && (m_textures[i].type != ID_DI) && (m_textures[i].type != ID_SI) && (m_textures[i].type != ID_RL)))
						continue;

					// do not use this textures for light pass
          if ((pass == ExShader::SP_LIGHT_MULTI) && ((m_textures[i].type == ID_SI) || isMask))
						continue;

					// don't export normal and specular maps for non supported technique
					if((pass == ExShader::SP_NOSUPPORT) && ((m_textures[i].type == ID_BU) || (m_textures[i].type == ID_SP) || isMask))
						continue;

          //ignore masked texture for basic mat
          if (pass == ExShader::SP_NOSUPPORT && (m_textures[i].hasMask || isMask))
          {
            isMask = m_textures[i].hasMask;
            continue;
          }            

					//start texture unit description
					outMaterial << "\n\t\t\ttexture_unit ";
					outMaterial << m_name << "_";
					switch (m_textures[i].type)
					{
					case ID_AM:
						outMaterial << "Ambient";
						break;
					case ID_DI:
						outMaterial << "Diffuse";
						break;
          case ID_SP:
            outMaterial << "Specular";
            break;
					case ID_BU:
						outMaterial << "Normal";
						break;
					case ID_SI:
						outMaterial << "Self-Illumination";
						break;
          case ID_RL:
            outMaterial << "Reflect";
            break;
					default:
						outMaterial << "Unknown";
					}
          if (isMask)
            outMaterial << "_Mask";

					outMaterial << "#" << (texUnitId++) << "\n";
					outMaterial << "\t\t\t{\n";

					//write texture name
          bool isCubic = false;

          if (m_textures[i].filename.size() > 1)
          {
            outMaterial << "\t\t\t\tanim_texture ";

            for (unsigned int k = 0; k < m_textures[i].filename.size(); k++)
            {
              std::string texName = params.resPrefix;
              texName.append(m_textures[i].filename[k]);
					    texName = optimizeFileName(texName);

					    //DDS conversion
					    if(params.convertToDDS)
					    {
						    texName = texName.substr(0, texName.find_last_of("."));
						    texName.append(".DDS");
					    }

              outMaterial << texName.c_str() << " ";
            }
            //duration
            float duration = (float)(m_textures[i].animRate * m_textures[i].filename.size()) / (float)GetFrameRate();
            outMaterial << duration;

            outMaterial << "\n";
          }
          else
          {
            std::string texName = params.resPrefix;
					  texName.append(m_textures[i].filename[0]);
					  texName = optimizeFileName(texName);

					  //DDS conversion
					  if(params.convertToDDS)
					  {
						  texName = texName.substr(0, texName.find_last_of("."));
						  texName.append(".DDS");
					  }

					  outMaterial << "\t\t\t\ttexture " << texName.c_str();

					  std::string texExt = ToLowerCase(m_textures[i].filename[0].substr(m_textures[i].filename[0].find_last_of(".") + 1));
					  if(texExt == "dds")
					  {
						  //detect if the texture is a cubemap
						  nv::DirectDrawSurface ddsMap(m_textures[i].absFilename[0].c_str());
						  if(ddsMap.isValid() && ddsMap.isTextureCube())
							  isCubic = true;
					  }
					  if(isCubic)
						  outMaterial << " cubic\n";
					  else
						  outMaterial << "\n";
          }

					//write texture coordinate index
					outMaterial << "\t\t\t\ttex_coord_set " << m_textures[i].uvsetIndex << "\n";

					// better texture quality
					if(((pass != ExShader::SP_NONE) && (pass != ExShader::SP_NOSUPPORT)) && ((m_textures[i].type == ID_DI) || (m_textures[i].type == ID_BU)))
						outMaterial << "\t\t\t\tmipmap_bias -1\n";

					//use anisotropic as default for better textures quality
					//outMaterial << "\t\t\t\tfiltering anisotropic\n";

          if ((m_textures[i].fAmount >= 1.0f) || (m_textures[i].type == ID_BU) || (m_textures[i].type == ID_SI) || (m_textures[i].type == ID_AM))
					{
            switch (m_textures[i].type)
            {
            case ID_AM:
            case ID_SI:
              outMaterial << "\t\t\t\tcolour_op add\n";
              break;
            default:
              outMaterial << "\t\t\t\tcolour_op modulate\n";
            }
					}
					else
					{
            outMaterial << "\t\t\t\tcolour_op_ex blend_manual src_texture src_current " << (((pass == ExShader::SP_NOSUPPORT) && (m_textures[i].type == ID_RL)) ? m_textures[i].fAmount / 3.0f : m_textures[i].fAmount) << "\n";
						outMaterial << "\t\t\t\tcolour_op_multipass_fallback one zero\n";
					}

					//remove alpha from ambient (common ligth map)
					if((m_textures[i].type == ID_AM) && (m_textures[i].bHasAlphaChannel))
						outMaterial << "\t\t\t\talpha_op_ex source2 src_manual src_current " << (m_textures[i].fAmount * m_opacity) << "\n";

					//write texture transforms
          if ((m_textures[i].scale_u != 1.0) || (m_textures[i].scale_v != 1.0))
					  outMaterial << "\t\t\t\tscale " << m_textures[i].scale_u << " " << m_textures[i].scale_v << "\n";
					if ((m_textures[i].scroll_u != 0.0) || (m_textures[i].scroll_v != 0.0))
            outMaterial << "\t\t\t\tscroll " << m_textures[i].scroll_u << " " << m_textures[i].scroll_v << "\n";
					if (m_textures[i].rot != 0.0)
            outMaterial << "\t\t\t\trotate " << m_textures[i].rot << "\n";

          if (m_textures[i].scroll_s_u != 0.0 || m_textures[i].scroll_s_v != 0.0)
            outMaterial << "\t\t\t\tscroll_anim " << m_textures[i].scroll_s_u << " " << m_textures[i].scroll_s_v << "\n";

          if (m_textures[i].rot_s != 0.0)
            outMaterial << "\t\t\t\trotate_anim " << m_textures[i].rot_s << "\n";

					if(m_textures[i].bReflect)
					{
						if(isCubic)
							outMaterial << "\t\t\t\tenv_map " << "cubic_reflection" << "\n";
						else
							outMaterial << "\t\t\t\tenv_map " << "planar" << "\n";
					}

					//end texture unit desription
					outMaterial << "\t\t\t}\n";

          isMask = m_textures[i].hasMask;
				}
			}
		}

		//End render pass description
		outMaterial << "\t\t}\n";
	}

};	//end namespace
