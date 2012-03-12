////////////////////////////////////////////////////////////////////////////////
// ExMaterial.h
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

#ifndef _MATERIAL_H
#define _MATERIAL_H

#include "ExPrerequisites.h"
#include "paramList.h"
#include "ExShader.h"

namespace EasyOgreExporter
{
	typedef enum {MT_SURFACE_SHADER, MT_LAMBERT, MT_PHONG, MT_BLINN, MT_CGFX,MT_FACETED} MaterialType;
	typedef enum {TOT_REPLACE, TOT_MODULATE, TOT_ADD, TOT_ALPHABLEND, TOT_MANUALBLEND} TexOpType;
	typedef enum {TAM_CLAMP, TAM_BORDER, TAM_WRAP, TAM_MIRROR} TexAddressMode;

  class MatProc : public PropertyEnum
  {
    public:
    MatProc();

    virtual bool Proc(IGameProperty * prop);
  };

	class Texture
	{
	public:
		//constructor
		Texture()
    {
			scale_u = scale_v = 1;
			scroll_u = scroll_v = 0;
			rot = 0;
			am_u = am_v = TAM_CLAMP;
      type = 0;
      uvsetIndex = 0;

			// Most textures like normal, specular, bump, etc. can't just
			// be summed into the diffuse channel and need
			bCreateTextureUnit = false;
      bReflect = false;
      fAmount = 1.0;
		}
		//destructor
		~Texture(){};
	
		//public members
		std::string filename;
		std::string absFilename;
		TexOpType opType;
		std::string uvsetName;
		int uvsetIndex;
		bool bCreateTextureUnit;
    bool bReflect;
    float fAmount;
		TexAddressMode am_u, am_v;
		double scale_u, scale_v;
		double scroll_u, scroll_v;
		double rot;
    int type;
	};


	/***** Class ExMaterial *****/
	class ExMaterial
	{
	public:		
    IGameMaterial* m_GameMaterial;
		std::string m_name;
		MaterialType m_type;
		Point4 m_ambient, m_diffuse, m_specular, m_emissive;
    float m_shininess;
    float m_opacity;
		bool m_lightingOff;
		bool m_isTransparent;
		bool m_isTextured;
    bool m_isTwoSided;
    bool m_isWire;
    bool m_hasAlpha;
    bool m_hasDiffuseMap;
    bool m_hasAmbientMap;
    bool m_hasSpecularMap;
    bool m_hasReflectionMap;
    bool m_hasBumpMap;
		std::vector<Texture> m_textures;
  private:
  protected:

  public:
		//constructor
		ExMaterial(IGameMaterial* pGameMaterial, std::string prefix);

		//destructor
		~ExMaterial();

		//get material name
		std::string& getName();

		//clear material data
		void clear();

		//load material data
		bool load(ParamList& params);

		//write material data to Ogre material script
		bool writeOgreScript(ParamList &params, std::ofstream &outMaterial, ExShader* vsShader, ExShader* fpShader);

		//copy textures to path specified by params
		bool copyTextures(ParamList &params);

    std::string getShaderName(ExShader::ShaderType type);
  private:
    void loadManualTexture(IGameProperty* prop, int type, float amount);
    void loadTextureUV(IGameTextureMap* pGameTexture, Texture &tex);
    void loadArchitectureMaterial(IGameMaterial* pGameMaterial);
    void loadArchAndDesignMaterial(IGameMaterial* pGameMaterial);
    void loadStandardMaterial(IGameMaterial* pGameMaterial);
    void writeMaterialTechnique(ParamList &params, std::ofstream &outMaterial, int lod, ExShader* vsShader, ExShader* fpShader);
    void writeMaterialPass(ParamList &params, std::ofstream &outMaterial, int lod, ExShader* vsShader, ExShader* fpShader);
		bool exportColor(Point4& color, IGameProperty* pGameProperty);
    bool exportSpecular(IGameMaterial* pGameMaterial);
    std::string getMaterialName(std::string prefix);
  protected:
	};

};	//end of namespace

#endif
