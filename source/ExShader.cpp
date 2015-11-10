////////////////////////////////////////////////////////////////////////////////
// ExShader.cpp
// Author   : Bastien BOURINEAU
// Start Date : March 10, 2012
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/

#include "ExShader.h"
#include "ExMaterial.h"
#include "ExTools.h"
#include "EasyOgreExporterLog.h"

namespace EasyOgreExporter
{
  // Common class ExShader
  ExShader::ExShader(std::string name)
	{
    m_name = name;
    m_type = ST_NONE;
    bNeedHighProfile = false;
  }

	ExShader::~ExShader()
	{
	}

  void ExShader::constructShader(ExMaterial* mat)
  {
  }

  void constructShaderGles(ExMaterial* mat)
  {
  }

	std::string& ExShader::getName()
	{
		return m_name;
	}

  ExShader::ShaderType ExShader::getType()
  {
    return m_type;
  }

	std::string& ExShader::getContent()
	{
		return m_content;
	}

  std::string& ExShader::getContentGles()
  {
    return m_contentGles;
  }

  std::string& ExShader::getUniformParams(ExMaterial* mat)
  {
    return m_params;
  }

  std::string& ExShader::getProgram(std::string baseName)
  {
    return m_program;
  }


// ExVsAmbShader
  ExVsAmbShader::ExVsAmbShader(std::string name) : ExShader(name)
	{
    m_type = ST_VSAM;
  }

	ExVsAmbShader::~ExVsAmbShader()
	{
	}

  void ExVsAmbShader::constructShader(ExMaterial* mat)
  {
    // get the material config
    bFresnel = (mat->m_type == MT_METAL) ? false : true;
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;

    std::vector<int> texUnits;
    for (int i = 0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
      {
        switch (mat->m_textures[i].type)
        {
          case ID_AM:
            texUnits.push_back(mat->m_textures[i].uvsetIndex);
          break;

          case ID_DI:
            texUnits.push_back(mat->m_textures[i].uvsetIndex);
          break;

          case ID_SI:
            texUnits.push_back(mat->m_textures[i].uvsetIndex);
          break;
        }
      }
    }

    std::sort(texUnits.begin(), texUnits.end());
    texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
    std::stringstream out;

    // generate the shader
    out << "void " << optimizeFileName(m_name).c_str() << "(float4 position	: POSITION,\n"; 
    out << "\tfloat3 normal : NORMAL,\n";

    for (int i=0; i < texUnits.size(); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texUnits[i] << ",\n";
    }

    int texCoord = 0;
    out << "\tout float4 oPos: POSITION,\n";

    if(bRef)
    {
      out << "\tout float3 oNorm : TEXCOORD" << texCoord++ << ",\n";
      out << "\tout float4 oWp : TEXCOORD" << texCoord++ << ",\n";
    }

    int lastAvailableTexCoord = texCoord;
    for (int i=0; i < texUnits.size() && (texCoord < 16); i++)
    {
      out << "\tout float2 oUv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    if(bRef)
    {
      out << "\tuniform float4x4 wMat,\n";
      out << "\tuniform float4x4 witMat,\n";
    }
    
    out << "\tuniform float4x4 wvpMat\n";

    out << ")\n";
    out << "{\n";

    out << "\toPos = mul(wvpMat, position);\n";
    if(bRef)
    {
      out << "\toWp = mul(wMat, position);\n";
      out << "\toNorm = normalize(mul((float3x3)witMat, normal));\n";
    }

    texCoord = lastAvailableTexCoord;
    for (int i=0; i < texUnits.size() && (texCoord < 16); i++)
    {
      out << "\toUv" << texUnits[i] << " = uv" << texUnits[i] << ";\n";
      texCoord++;
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    out << "}\n";
    m_content = out.str();
	}

  std::string& ExVsAmbShader::getUniformParams(ExMaterial* mat)
  {
    std::stringstream out;
    out << "\t\t\tvertex_program_ref " << m_name << "\n";
		out << "\t\t\t{\n";
		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExVsAmbShader::getProgram(std::string baseName)
  {
    std::stringstream out;
    out << "vertex_program " << m_name << " cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    if (bNeedHighProfile)
      out << "\tprofiles vs_4_0 arbvp2\n";
    else
      out << "\tprofiles vs_1_1 arbvp1\n";
    out << "\tentry_point " << optimizeFileName(m_name) << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto wvpMat worldviewproj_matrix\n";
    
    if(bRef)
    {
      out << "\t\tparam_named_auto witMat inverse_transpose_world_matrix\n";
      out << "\t\tparam_named_auto wMat world_matrix\n";
    }
    out << "\t}\n";
    out << "}\n";

    m_program = out.str();
    return m_program;
  }


  // ExFpAmbShader
  ExFpAmbShader::ExFpAmbShader(std::string name) : ExShader(name)
	{
    m_type = ST_FPAM;
  }

	ExFpAmbShader::~ExFpAmbShader()
	{
	}

  void ExFpAmbShader::constructShader(ExMaterial* mat)
  {
    // get the material config
    bFresnel = (mat->m_type == MT_METAL) ? false : true;
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;
    bAmbient = mat->m_hasAmbientMap;
    bool bIllum = false;

    std::vector<int> texUnits;
    for (int i = 0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
      {
        switch (mat->m_textures[i].type)
        {
          case ID_AM:
            texUnits.push_back(mat->m_textures[i].uvsetIndex);
          break;

          case ID_DI:
            texUnits.push_back(mat->m_textures[i].uvsetIndex);
          break;

          case ID_SI:
            bIllum = true;
            texUnits.push_back(mat->m_textures[i].uvsetIndex);
          break;
        }
      }
    }

    std::sort(texUnits.begin(), texUnits.end());
    texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
    std::stringstream out;

    int texCoord = 0;
    // generate the shader
    out << "float4 " << optimizeFileName(m_name).c_str() << "(float4 position	: POSITION,\n";
    
    if(bRef)
    {
      out << "\tfloat3 normal : TEXCOORD" << texCoord++ << ",\n";
      out << "\tfloat4 wp : TEXCOORD" << texCoord++ << ",\n";
    }

    for (int i=0; i < texUnits.size() && (texCoord < 16); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    out << "\tuniform float3 ambient,\n";
    out << "\tuniform float4 matAmb,\n";
    out << "\tuniform float4 matEmissive";

    if(bRef)
    {
      out << ",\n\tuniform float3 camPos";
      out << ",\n\tuniform float reflectivity";

      if (bFresnel)
      {
        out << ",\n\tuniform float fresnelMul";
        out << ",\n\tuniform float fresnelPow";
      }
    }

    int samplerId = 0;
    int ambUv = 0;
    int diffUv = 0;
    int illUv = 0;

    for (int i=0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
	    {
        switch (mat->m_textures[i].type)
        {
          case ID_AM:
            out << ",\n\tuniform sampler2D ambMap : register(s" << samplerId << ")";
            ambUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_DI:
            out << ",\n\tuniform sampler2D diffuseMap : register(s" << samplerId << ")";
            diffUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_SI:
            out << ",\n\tuniform sampler2D illMap : register(s" << samplerId << ")";
            illUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_RL:
            out << ",\n\tuniform samplerCUBE reflectMap : register(s" << samplerId << ")";
            samplerId++;
          break;
        }
      }
    }
    
    out << "): COLOR0\n";
    out << "{\n";

    if(bAmbient)
      out << "\tfloat4 ambTex = tex2D(ambMap, uv" << ambUv << ");\n";

    if(bDiffuse)
      out << "\tfloat4 diffuseTex = tex2D(diffuseMap, uv" << diffUv << ");\n";
    
    if(bIllum)
      out << "\tfloat4 illTex = tex2D(illMap, uv" << illUv << ");\n";
    
    out << "\tfloat4 retColor = max(matEmissive, float4(ambient, 1) * matAmb);\n";
    
    if(bAmbient)
      out << "\tretColor *= float4(ambTex.rgb, 1.0);\n";

    if(bDiffuse)
      out << "\tretColor *= diffuseTex;\n";

    if(bIllum)
      out << "\tretColor = max(retColor, illTex);\n";

    if(bRef)
    {
      out << "\tfloat3 camDir = normalize(camPos - wp.xyz);\n";
      out << "\tfloat3 refVec = -reflect(camDir, normal);\n";
      out << "\trefVec.z = -refVec.z;\n";
      out << "\tfloat4 reflecTex = texCUBE(reflectMap, refVec);\n";

      if (bFresnel)
      {
        out << "\tfloat fresnel = fresnelMul * reflectivity * pow(1 + dot(-camDir, normal), fresnelPow - (reflectivity * fresnelMul));\n";
        out << "\tfloat4 reflecVal = reflecTex * fresnel;\n";
      }
      else
      {
        out << "\tfloat4 reflecVal = reflecTex * reflectivity;\n";
      }

      out << "\tretColor += float4(ambient, 1) * matAmb * reflecVal;\n";
    }

    out << "\treturn retColor;\n";

    out << "}\n";
    m_content = out.str();
	}

  std::string& ExFpAmbShader::getUniformParams(ExMaterial* mat)
  {
    std::stringstream out;
    out << "\t\t\tfragment_program_ref " << m_name << "\n";
		out << "\t\t\t{\n";

    if(bRef)
    {
      out << "\t\t\t\tparam_named reflectivity float " << mat->m_reflectivity << "\n";
      if (bFresnel)
      {
        out << "\t\t\t\tparam_named fresnelMul float " << mat->m_reflectivity << "\n";
        out << "\t\t\t\tparam_named fresnelPow float " << (mat->m_softness * 4.0f) << "\n";
      }
    }

		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExFpAmbShader::getProgram(std::string baseName)
  {
    std::stringstream out;
    out << "fragment_program " << m_name << " cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    if (bNeedHighProfile)
      out << "\tprofiles ps_3_x arbfp1\n";
    else
      out << "\tprofiles ps_3_0 arbfp1\n";
    out << "\tentry_point " << optimizeFileName(m_name) << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto ambient ambient_light_colour\n";
    out << "\t\tparam_named_auto matAmb surface_ambient_colour\n";
    out << "\t\tparam_named_auto matEmissive surface_emissive_colour\n";
    
    if(bRef)
    {
      out << "\t\tparam_named_auto camPos camera_position\n";
      out << "\t\tparam_named reflectivity float 1.0\n";

      if (bFresnel)
      {
        out << "\t\tparam_named fresnelMul float 1.0\n";
        out << "\t\tparam_named fresnelPow float 1.0\n";
      }
    }

    out << "\t}\n";
    out << "}\n";

    m_program = out.str();
    return m_program;
  }


  // ExVsLightShader
  ExVsLightShader::ExVsLightShader(std::string name) : ExShader(name)
	{
    m_type = ST_VSLIGHT;
  }

	ExVsLightShader::~ExVsLightShader()
	{
	}

  void ExVsLightShader::constructShader(ExMaterial* mat)
  {
    // get the material config
    bFresnel = (mat->m_type == MT_METAL) ? false : true;
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;

    std::vector<int> texUnits;
    for (int i = 0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
        texUnits.push_back(mat->m_textures[i].uvsetIndex);
    }

    std::sort(texUnits.begin(), texUnits.end());
    texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
    std::stringstream out;

    // generate the shader
    out << "void " << optimizeFileName(m_name).c_str() << "(float4 position	: POSITION,\n"; 
    out << "\tfloat3 normal : NORMAL,\n";
    if(bNormal)
    {
      out << "\tfloat3 tangent : TANGENT,\n";
    }

    for (int i=0; i < texUnits.size(); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texUnits[i] << ",\n";
    }

    int texCoord = 0;
    out << "\tout float4 oPos : POSITION,\n";
    out << "\tout float3 oNorm : TEXCOORD" << texCoord++ << ",\n";

    if(bNormal)
    {
      out << "\tout float3 oTang : TEXCOORD" << texCoord++ << ",\n";
      out << "\tout float3 oBinormal : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tout float3 oSpDir0 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tout float3 oSpDir1 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tout float3 oSpDir2 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tout float4 oWp : TEXCOORD" << texCoord++ << ",\n";

    int lastAvailableTexCoord = texCoord;
    int lastUvIndex = -1;

    for (int i=0; i < texUnits.size() && (texCoord < 16); i++)
    {
      if (!(texUnits[i] % 2))
      {
        out << "\tout float4 oUv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
        lastUvIndex = texUnits[i];
      }
      else
      {
        if (lastUvIndex != texUnits[i] - 1)
          out << "\tout float4 oUv" << texUnits[i] - 1 << " : TEXCOORD" << texCoord++ << ",\n";
        lastUvIndex = texUnits[i] - 1;
      }
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    out << "\tuniform float4x4 wMat,\n";

    if(!bNormal)
      out << "\tuniform float4x4 witMat,\n";
    
    out << "\tuniform float4x4 wvpMat,\n";
    out << "\tuniform float4 spotlightDir0,\n";
    out << "\tuniform float4 spotlightDir1,\n";
    out << "\tuniform float4 spotlightDir2";

    out << ")\n";
    out << "{\n";

    out << "\toWp = mul(wMat, position);\n";
    out << "\toPos = mul(wvpMat, position);\n";

    texCoord = lastAvailableTexCoord;
    lastUvIndex = -1;
    for (int i=0; i < texUnits.size() && (texCoord < 16); i++)
    {
      if (!(texUnits[i] % 2))
      {
        out << "\toUv" << texUnits[i] << ".xy = uv" << texUnits[i] << ";\n";
        lastUvIndex = texUnits[i];
        texCoord++;
      }
      else
      {
        if ((texUnits[i] - 1) == lastUvIndex)
        {
          out << "\toUv" << texUnits[i] - 1 << ".zw = uv" << texUnits[i] << ";\n";
        }
        else
        {
          out << "\toUv" << texUnits[i] - 1 << " = float4(0.0, 0.0, uv" << texUnits[i] << ");\n";
          texCoord++;
        }

        lastUvIndex = texUnits[i] - 1;
      }
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    if(bNormal)
    {
      out << "\toTang = tangent;\n";
      out << "\toBinormal = cross(tangent, normal);\n";
      out << "\toNorm = normal;\n";
    }
    else
    {
      out << "\toNorm = normalize(mul((float3x3)witMat, normal));\n";
    }

    out << "\toSpDir0 = mul(wMat, spotlightDir0).xyz;\n";
    out << "\toSpDir1 = mul(wMat, spotlightDir1).xyz;\n";
    out << "\toSpDir2 = mul(wMat, spotlightDir2).xyz;\n";
    out << "}\n";
    m_content = out.str();
	}

  void ExVsLightShader::constructShaderGles(ExMaterial* mat)
  {
    // get the material config
    bFresnel = (mat->m_type == MT_METAL) ? false : true;
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;

    std::vector<int> texUnits;
    for (int i = 0; i < mat->m_textures.size(); i++)
    {
      if (mat->m_textures[i].bCreateTextureUnit == true)
        texUnits.push_back(mat->m_textures[i].uvsetIndex);
    }

    std::sort(texUnits.begin(), texUnits.end());
    texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
    std::stringstream out;

    // generate the shader
    out << "#version 100\n";
    out << "precision mediump int;\n";
    out << "precision highp float;\n";

    // attributes
    out << "attribute vec4 position;\n";
    out << "attribute vec3 normal;\n";
    if (bNormal)
    {
      out << "attribute vec4 tangent;\n";
    }

    for (int i = 0; i < texUnits.size(); i++)
    {
      out << "attribute vec uv" << texUnits[i] << ";\n";
    }

    //uniform
    out << "uniform mat4 wMat;\n";

    if (!bNormal)
      out << "uniform mat4 witMat;\n";

    out << "uniform mat4 wvpMat;\n";
    out << "uniform vec4 spotlightDir0;\n";
    out << "uniform vec4 spotlightDir1;\n";
    out << "uniform vec4 spotlightDir2;";

    // varying
    int texCoord = 0;
    out << "\tvarying vec3 oNorm;\n";

    if (bNormal)
    {
      out << "\tvarying vec3 oTang;\n";
      out << "\tvarying vec3 oBinormal;\n";
    }

    out << "\tvarying vec3 oSpDir0;\n";
    out << "\tvarying vec3 oSpDir1;\n";
    out << "\tvarying vec3 oSpDir2;\n";
    out << "\tvarying vec4 oWp;\n";

    int lastAvailableTexCoord = texCoord;
    int lastUvIndex = -1;

    for (int i = 0; i < texUnits.size() && (texCoord < 16); i++)
    {
      if (!(texUnits[i] % 2))
      {
        out << "\tvarying vec4 oUv" << texUnits[i] << ";\n";
        lastUvIndex = texUnits[i];
        texCoord++;
      }
      else
      {
        if (lastUvIndex != texUnits[i] - 1)
        {
          out << "\tvarying vec4 oUv" << texUnits[i] - 1 << ";\n";
          texCoord++;
        }
        lastUvIndex = texUnits[i] - 1;
      }
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    // main start
    out << "void main()\n";
    out << "{\n";

    out << "\toWp = wMat * position;\n";
    out << "\tgl_Position = wvpMat * position;\n";

    texCoord = lastAvailableTexCoord;
    lastUvIndex = -1;
    for (int i = 0; i < texUnits.size() && (texCoord < 16); i++)
    {
      if (!(texUnits[i] % 2))
      {
        out << "\toUv" << texUnits[i] << ".xy = uv" << texUnits[i] << ";\n";
        lastUvIndex = texUnits[i];
        texCoord++;
      }
      else
      {
        if ((texUnits[i] - 1) == lastUvIndex)
        {
          out << "\toUv" << texUnits[i] - 1 << ".zw = uv" << texUnits[i] << ";\n";
        }
        else
        {
          out << "\toUv" << texUnits[i] - 1 << " = vec4(0.0, 0.0, uv" << texUnits[i] << ");\n";
          texCoord++;
        }

        lastUvIndex = texUnits[i] - 1;
      }
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    if (bNormal)
    {
      out << "\toTang = tangent.xyz;\n";
      out << "\toBinormal = cross(normal, tangent.xyz) * tangent.www;\n";
      out << "\toNorm = normal;\n";
    }
    else
    {
      out << "\toNorm = normalize(mat3(witMat) * normal);\n";
    }

    out << "\toSpDir0 = (wMat * spotlightDir0).xyz;\n";
    out << "\toSpDir1 = (wMat * spotlightDir1).xyz;\n";
    out << "\toSpDir2 = (wMat * spotlightDir2).xyz;\n";
    out << "}\n";
    m_contentGles = out.str();
  }

  std::string& ExVsLightShader::getUniformParams(ExMaterial* mat)
  {
    std::stringstream out;
    out << "\t\t\tvertex_program_ref " << m_name << "\n";
		out << "\t\t\t{\n";
		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExVsLightShader::getProgram(std::string baseName)
  {
    std::stringstream out;

    //CG
    out << "vertex_program " << m_name << "_CG cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    if (bNeedHighProfile)
      out << "\tprofiles vs_4_0 arbvp2\n";
    else
      out << "\tprofiles vs_1_1 arbvp1\n";
    out << "\tentry_point " << optimizeFileName(m_name) << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto wMat world_matrix\n";

    if(!bNormal)
      out << "\t\tparam_named_auto witMat inverse_transpose_world_matrix\n";

    out << "\t\tparam_named_auto wvpMat worldviewproj_matrix\n";
    out << "\t\tparam_named_auto spotlightDir0 light_direction_object_space 0\n";
    out << "\t\tparam_named_auto spotlightDir1 light_direction_object_space 1\n";
    out << "\t\tparam_named_auto spotlightDir2 light_direction_object_space 2\n";

    out << "\t}\n";
    out << "}\n";

    //GLSLES
    out << "vertex_program " << m_name << "_GLSLES glsles\n";
    out << "{\n";

    out << "\tsource " << optimizeFileName(m_name) << "VP.glsles\n";
    out << "\tprofiles glsles\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto wMat world_matrix\n";

    if (!bNormal)
      out << "\t\tparam_named_auto witMat inverse_transpose_world_matrix\n";

    out << "\t\tparam_named_auto wvpMat worldviewproj_matrix\n";
    out << "\t\tparam_named_auto spotlightDir0 light_direction_object_space 0\n";
    out << "\t\tparam_named_auto spotlightDir1 light_direction_object_space 1\n";
    out << "\t\tparam_named_auto spotlightDir2 light_direction_object_space 2\n";

    out << "\t}\n";
    out << "}\n";

    //DELEGATE
    out << "vertex_program " << m_name << " unified\n";
    out << "{\n";

    out << "\tdelegate " << m_name << "_CG\n";
    out << "\tdelegate " << m_name << "_GLSLES\n";

    out << "}\n";

    m_program = out.str();
    return m_program;
  }


  // ExFpLightShader
  ExFpLightShader::ExFpLightShader(std::string name) : ExShader(name)
	{
    m_type = ST_FPLIGHT;
  }

	ExFpLightShader::~ExFpLightShader()
	{
	}

  void ExFpLightShader::constructShader(ExMaterial* mat)
  {
    // get the material config
    bFresnel = (mat->m_type == MT_METAL) ? false : true;
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;
    bAmbient = mat->m_hasAmbientMap;
    bool bIllum = false;

    std::vector<int> texUnits;
    for (int i = 0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
	    {
        texUnits.push_back(mat->m_textures[i].uvsetIndex);
      }
    }

    std::sort(texUnits.begin(), texUnits.end());
    texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
    std::stringstream out;

    int texCoord = 0;
    // generate the shader
    out << "float4 " << optimizeFileName(m_name).c_str() << "(float4 position	: POSITION,\n";
    out << "\tfloat3 norm : TEXCOORD" << texCoord++ << ",\n";

    if(bNormal)
    {
      out << "\tfloat3 tangent : TEXCOORD" << texCoord++ << ",\n";
      out << "\tfloat3 binormal : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tfloat3 spDir0 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat3 spDir1 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat3 spDir2 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat4 wp : TEXCOORD" << texCoord++ << ",\n";

    int lastUvIndex = -1;
    for (int i=0; i < texUnits.size() && (texCoord < 16); i++)
    {
      if (!(texUnits[i] % 2))
      {
        out << "\tfloat4 uv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
        lastUvIndex = texUnits[i];
      }
      else
      {
        // only if not added yet
        if (lastUvIndex != texUnits[i] - 1)
          out << "\tfloat4 uv" << texUnits[i] - 1 << " : TEXCOORD" << texCoord++ << ",\n";

        lastUvIndex = texUnits[i] - 1;
      }
    }

    if (texCoord > 8)
      bNeedHighProfile = true;

    out << "\tuniform float3 ambient,\n";
    out << "\tuniform float3 lightDif0,\n";
    out << "\tuniform float4 lightPos0,\n";
    out << "\tuniform float4 lightAtt0,\n";
    out << "\tuniform float3 lightSpec0,\n";
    out << "\tuniform float3 lightDif1,\n";
    out << "\tuniform float4 lightPos1,\n";
    out << "\tuniform float4 lightAtt1,\n";
    out << "\tuniform float3 lightSpec1,\n";
    out << "\tuniform float3 lightDif2,\n";
    out << "\tuniform float4 lightPos2,\n";
    out << "\tuniform float4 lightAtt2,\n";
    out << "\tuniform float3 lightSpec2,\n";
    out << "\tuniform float3 camPos,\n";
    out << "\tuniform float4 matAmb,\n";
    out << "\tuniform float4 matEmissive,\n";
    out << "\tuniform float4 matDif,\n";
    out << "\tuniform float4 matSpec,\n";
    out << "\tuniform float matShininess,\n";
    out << "\tuniform float4 invSMSize,\n";
    out << "\tuniform float4 spotlightParams0,\n";
    out << "\tuniform float4 spotlightParams1,\n";
    out << "\tuniform float4 spotlightParams2";
    if(bNormal)
    {
      out << ",\n\tuniform float4x4 iTWMat";
      out << ",\n\tuniform float normalMul";
    }

    if(bRef)
    {
      out << ",\n\tuniform float reflectivity";

      if (bFresnel)
      {
        out << ",\n\tuniform float fresnelMul";
        out << ",\n\tuniform float fresnelPow";
      }
    }

    int samplerId = 0;
    int ambUv = 0;
    int diffUv = 0;
    int specUv = 0;
    int normUv = 0;
    int illUv = 0;

    for (int i=0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
	    {
        switch (mat->m_textures[i].type)
        {
          //could be a light map
          case ID_AM:
            out << ",\n\tuniform sampler2D ambMap : register(s" << samplerId << ")";
            ambUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_DI:
            out << ",\n\tuniform sampler2D diffuseMap : register(s" << samplerId << ")";
            diffUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_SP:
            out << ",\n\tuniform sampler2D specMap : register(s" << samplerId << ")";
            specUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_BU:
            out << ",\n\tuniform sampler2D normalMap : register(s" << samplerId << ")";
            normUv = mat->m_textures[i].uvsetIndex;
            samplerId++;
          break;

          case ID_SI:
            out << ",\n\tuniform sampler2D illMap : register(s" << samplerId << ")";
            illUv = mat->m_textures[i].uvsetIndex;
            bIllum = true;
            samplerId++;
            break;

          case ID_RL:
            out << ",\n\tuniform samplerCUBE reflectMap : register(s" << samplerId << ")";
            samplerId++;
          break;
        }
      }
    }
    
    out << "): COLOR0\n";
    out << "{\n";

    if (bNormal)
    {
      out << "\tfloat3 normalTex = tex2D(normalMap, uv";
      if (!(normUv % 2))
        out << normUv << ".xy" << ").rgb;\n";
      else
        out << normUv - 1 << ".zw" << ").rgb;\n";

      out << "\ttangent *= normalMul;\n";
      out << "\tbinormal *= normalMul;\n";
      out << "\tfloat3x3 tbn = float3x3(tangent, binormal, norm);\n";
      out << "\tfloat3 normal = mul(transpose(tbn), (normalTex.xyz -0.5) * 2); // to object space\n";
      out << "\tnormal = normalize(mul((float3x3)iTWMat, normal));\n";
    }
    else
    {
      out << "\tfloat3 normal = normalize(norm);\n";
    }

    // direction
    out << "\tfloat3 ld0 = normalize(lightPos0.xyz - (lightPos0.w * wp.xyz));\n";
    out << "\tfloat3 ld1 = normalize(lightPos1.xyz - (lightPos1.w * wp.xyz));\n";
    out << "\tfloat3 ld2 = normalize(lightPos2.xyz - (lightPos2.w * wp.xyz));\n";

    out << "\t// attenuation\n";
    out << "\thalf lightDist = length(lightPos0.xyz - wp.xyz) / (lightAtt0.r / lightAtt0.r);\n";
    out << "\thalf la0 = 1;\n";
    out << "\thalf ila = 0;\n";
    out << "\tif(lightAtt0.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\tila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla0 = 1.0 / (lightAtt0.g + lightAtt0.b * lightDist + lightAtt0.a * ila);\n";
    out << "\t}\n";

    out << "\t// attenuation\n";
    out << "\tlightDist = length(lightPos1.xyz - wp.xyz) / (lightAtt1.r / lightAtt1.r);\n";
    out << "\thalf la1 = 1;\n";
    out << "\tif(lightAtt1.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\tila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla1 = 1.0 / (lightAtt1.g + lightAtt1.b * lightDist + lightAtt1.a * ila);\n";
    out << "\t}\n";

    out << "\t// attenuation\n";
    out << "\tlightDist = length(lightPos2.xyz - wp.xyz) / (lightAtt2.r / lightAtt2.r);\n";
    out << "\thalf la2 = 1;\n";
    out << "\tif(lightAtt2.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\tila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla2 = 1.0 / (lightAtt2.g + lightAtt2.b * lightDist + lightAtt2.a * ila);\n";
    out << "\t}\n";

    out << "\tfloat3 diffuse0 = max(dot(normal, ld0), 0) * lightDif0;\n";
    out << "\tfloat3 diffuse1 = max(dot(normal, ld1), 0) * lightDif1;\n";
    out << "\tfloat3 diffuse2 = max(dot(normal, ld2), 0) * lightDif2;\n";

    out << "\t// calculate the spotlight effect\n";
    out << "\tfloat spot0 = (spotlightParams0.x == 1 && spotlightParams0.y == 0 && spotlightParams0.z == 0 && spotlightParams0.w == 1 ? 1 : // if so, then it's not a spot light\n";
    out << "\t   saturate((dot(normalize(-spDir0), ld0) - spotlightParams0.y) / (spotlightParams0.x - spotlightParams0.y)));\n";
    out << "\tfloat spot1 = (spotlightParams1.x == 1 && spotlightParams1.y == 0 && spotlightParams1.z == 0 && spotlightParams1.w == 1 ? 1 : // if so, then it's not a spot light\n";
    out << "\t   saturate((dot(normalize(-spDir1), ld1) - spotlightParams1.y) / (spotlightParams1.x - spotlightParams1.y)));\n";
    out << "\tfloat spot2 = (spotlightParams2.x == 1 && spotlightParams2.y == 0 && spotlightParams2.z == 0 && spotlightParams2.w == 1 ? 1 : // if so, then it's not a spot light\n";
    out << "\t   saturate((dot(normalize(-spDir2), ld2) - spotlightParams2.y) / (spotlightParams2.x - spotlightParams2.y)));\n";

    out << "\tfloat3 camDir = normalize(camPos - wp.xyz);\n";
    out << "\tfloat3 halfVec = normalize(ld0 + camDir);\n";
    out << "\tfloat3 specularLight = pow(max(dot(normal, halfVec), 0), matShininess) * lightSpec0;\n";
    out << "\thalfVec = normalize(ld1 + camDir);\n";
    out << "\tspecularLight += pow(max(dot(normal, halfVec), 0), matShininess) * lightSpec1;\n";
    out << "\thalfVec = normalize(ld2 + camDir);\n";
    out << "\tspecularLight += pow(max(dot(normal, halfVec), 0), matShininess) * lightSpec2;\n";

    out << "\tfloat3 diffuseLight = (diffuse0 * spot0 * la0) + (diffuse1 * spot1 * la1) + (diffuse2 * spot2 * la2);\n";
    out << "\tfloat3 ambientColor = max(matEmissive.rgb, ambient * matAmb.rgb);\n";

    out << "\tfloat3 diffuseContrib = matDif.rgb;\n";

    if (bDiffuse)
    {
      out << "\tfloat4 diffuseTex = tex2D(diffuseMap, uv";
      if (!(diffUv % 2))
        out << diffUv << ".xy" << ");\n";
      else
        out << diffUv - 1 << ".zw" << ");\n";

      out << "\tambientColor *= diffuseTex.rgb;\n";
      out << "\tdiffuseContrib *= diffuseTex.rgb;\n";
    }

    if (bAmbient)
    {
      out << "\tfloat3 ambTex = tex2D(ambMap, uv";
      if (!(ambUv % 2))
        out << ambUv << ".xy" << ").rgb;\n";
      else
        out << ambUv - 1 << ".zw" << ").rgb;\n";
      out << "\tambientColor *= ambTex;\n";
    }
    
    if (bIllum)
    {
      out << "\tfloat4 illTex = tex2D(illMap, uv";
      if (!(illUv % 2))
        out << illUv << ".xy" << ").rgb;\n";
      else
        out << illUv - 1 << ".zw" << ").rgb;\n";
      out << "\tambientColor = max(ambientColor, illTex.rgb);\n";
    }

    out << "\tfloat3 specularContrib = specularLight * matSpec.rgb;\n";

    if (bSpecular)
    {
      out << "\tfloat4 specTex = tex2D(specMap, uv";
      if (!(specUv % 2))
        out << specUv << ".xy" << ");\n";
      else
        out << specUv - 1 << ".zw" << ");\n";
      out << "\tspecularContrib *= specTex.rgb;\n";
    }      

    //if(bAmbient)
      //out << "\tspecularContrib *= ambTex.rgb;\n";

    out << "\tfloat3 light0C = ambientColor + (diffuseLight * diffuseContrib) + specularContrib;\n";

    out << "\tfloat alpha = matDif.a;\n";
    if(bDiffuse)
      out << "\talpha *= diffuseTex.a;\n";
    else if(bAmbient)
      out << "\talpha *= ambTex.a;\n";

    if(bRef)
    {
      out << "\tfloat3 refVec = -reflect(camDir, normal);\n";
      out << "\trefVec.z = -refVec.z;\n";
      out << "\tfloat4 reflecTex = texCUBE(reflectMap, refVec);\n";

      if (bFresnel)
      {
        out << "\tfloat fresnel = fresnelMul * reflectivity * pow(1 + dot(-camDir, normal), fresnelPow - (reflectivity * fresnelMul));\n";
        out << "\tfloat4 reflecVal = reflecTex * fresnel;\n";
        out << "\tfloat3 reflectColor = reflecVal.rgb * (1.0 - light0C);\n";
      }
      else //metal style
      {
        out << "\tfloat3 reflectColor = reflecTex.rgb * reflectivity;\n";
      }
      
      out << "\treturn float4(light0C + reflectColor, alpha);\n";
    }
    else
      out << "\treturn float4(light0C, alpha);\n";

    out << "}\n";
    m_content = out.str();
	}

  void ExFpLightShader::constructShaderGles(ExMaterial* mat)
  {
    // get the material config
    bFresnel = (mat->m_type == MT_METAL) ? false : true;
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;
    bAmbient = mat->m_hasAmbientMap;
    bool bIllum = false;

    std::vector<int> texUnits;
    for (int i = 0; i < mat->m_textures.size(); i++)
    {
      if (mat->m_textures[i].bCreateTextureUnit == true)
      {
        texUnits.push_back(mat->m_textures[i].uvsetIndex);
      }
    }

    std::sort(texUnits.begin(), texUnits.end());
    texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
    std::stringstream out;

    /////////////
    // generate the shader
    out << "#version 100\n";
    out << "precision mediump int;\n";
    out << "precision highp float;\n";

    //uniform
    out << "uniform vec3 ambient;\n";
    out << "uniform vec3 lightDif0;\n";
    out << "uniform vec4 lightPos0;\n";
    out << "uniform vec4 lightAtt0;\n";
    out << "uniform vec3 lightSpec0;\n";
    out << "uniform vec3 lightDif1;\n";
    out << "uniform vec4 lightPos1;\n";
    out << "uniform vec4 lightAtt1;\n";
    out << "uniform vec3 lightSpec1;\n";
    out << "uniform vec3 lightDif2;\n";
    out << "uniform vec4 lightPos2;\n";
    out << "uniform vec4 lightAtt2;\n";
    out << "uniform vec3 lightSpec2;\n";
    out << "uniform vec3 camPos;\n";
    out << "uniform vec4 matAmb;\n";
    out << "uniform vec4 matEmissive;\n";
    out << "uniform vec4 matDif;\n";
    out << "uniform vec4 matSpec;\n";
    out << "uniform float matShininess;\n";
    out << "uniform vec4 invSMSize;\n";
    out << "uniform vec4 spotlightParams0;\n";
    out << "uniform vec4 spotlightParams1;\n";
    out << "uniform vec4 spotlightParams2;\n";
    if (bNormal)
    {
      out << "uniform mat4 iTWMat;\n";
      out << "uniform float normalMul;\n";
    }

    if (bRef)
    {
      out << "uniform float reflectivity;";

      if (bFresnel)
      {
        out << "uniform float fresnelMul;";
        out << "uniform float fresnelPow;";
      }
    }

    int samplerId = 0;
    int ambUv = 0;
    int diffUv = 0;
    int specUv = 0;
    int normUv = 0;
    int illUv = 0;

    for (int i = 0; i < mat->m_textures.size(); i++)
    {
      if (mat->m_textures[i].bCreateTextureUnit == true)
      {
        switch (mat->m_textures[i].type)
        {
          //could be a light map
        case ID_AM:
          out << "uniform sampler2D ambMap;\n";
          ambUv = mat->m_textures[i].uvsetIndex;
          samplerId++;
          break;

        case ID_DI:
          out << "uniform sampler2D diffuseMap;\n";
          diffUv = mat->m_textures[i].uvsetIndex;
          samplerId++;
          break;

        case ID_SP:
          out << "uniform sampler2D specMap;\n";
          specUv = mat->m_textures[i].uvsetIndex;
          samplerId++;
          break;

        case ID_BU:
          out << "uniform sampler2D normalMap;\n";
          normUv = mat->m_textures[i].uvsetIndex;
          samplerId++;
          break;

        case ID_SI:
          out << "uniform sampler2D illMap;\n";
          illUv = mat->m_textures[i].uvsetIndex;
          bIllum = true;
          samplerId++;
          break;

        case ID_RL:
          out << "uniform samplerCUBE reflectMap;\n";
          samplerId++;
          break;
        }
      }
    }

    // varying
    int texCoord = 0;
    out << "\tvarying vec3 oNorm;\n";

    if (bNormal)
    {
      out << "\tvarying vec3 oTang;\n";
      out << "\tvarying vec3 oBinormal;\n";
    }

    out << "\tvarying vec3 oSpDir0;\n";
    out << "\tvarying vec3 oSpDir1;\n";
    out << "\tvarying vec3 oSpDir2;\n";
    out << "\tvarying vec4 oWp;\n";

    int lastAvailableTexCoord = texCoord;
    int lastUvIndex = -1;

    for (int i = 0; i < texUnits.size() && (texCoord < 16); i++)
    {
      if (!(texUnits[i] % 2))
      {
        out << "\tvarying vec4 oUv" << texUnits[i] << ";\n";
        lastUvIndex = texUnits[i];
        texCoord++;
      }
      else
      {
        if (lastUvIndex != texUnits[i] - 1)
        {
          out << "\tvarying vec4 oUv" << texUnits[i] - 1 << ";\n";
          texCoord++;
        }
        lastUvIndex = texUnits[i] - 1;
      }
    }

    if (bNormal)
    {
      out << "highp mat3 transposeMat3(in highp mat3 inMatrix) {\n";
      out << "highp vec3 i0 = inMatrix[0];\n";
      out << "highp vec3 i1 = inMatrix[1];\n";
      out << "highp vec3 i2 = inMatrix[2];\n";

      out << "highp mat3 outMatrix = mat3(\n";
      out << "vec4(i0.x, i1.x, i2.x),\n";
      out << "vec4(i0.y, i1.y, i2.y),\n";
      out << "vec4(i0.z, i1.z, i2.z)\n";
      out << ");\n";
      out << "return outMatrix;}\n";
    }

    // generate the shader
    // main start
    out << "void main()\n";
    out << "{\n";
    /*
    out << "\tfloat3 normal : TEXCOORD" << texCoord++ << ",\n";

    if (bNormal)
    {
      out << "\tfloat3 tangent : TEXCOORD" << texCoord++ << ",\n";
      out << "\tfloat3 binormal : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tfloat3 spDir0 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat3 spDir1 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat3 spDir2 : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat4 wp : TEXCOORD" << texCoord++ << ",\n";

    out << "): COLOR0\n";
    out << "{\n";
    */
    if (bNormal)
    {
      out << "\tfloat3 normalTex = texture2D(normalMap, uv";
      if (!(normUv % 2))
        out << normUv << ".xy" << ").xyz;\n";
      else
        out << normUv - 1 << ".zw" << ").xyz;\n";

      out << "\ttangent *= normalMul;\n";
      out << "\tbinormal *= normalMul;\n";
      out << "\tmat3 tbn = mat3(oTang, oBinormal, oNorm);\n";
      out << "\tvec3 normal = transposeMat3(tbn) * ((normalTex.xyz - vec3(0.5)) * 2); // to object space\n";
      out << "\tnormal = normalize(mat3(iTWMat) * normal);\n";
    }
    else
    {
      out << "\tvec3 normal = normalize(oNorm);\n";
    }

    // direction
    out << "\tvec3 ld0 = normalize(lightPos0.xyz - (vec3(lightPos0.w) * oWp.xyz));\n";
    out << "\tvec3 ld1 = normalize(lightPos1.xyz - (vec3(lightPos1.w) * oWp.xyz));\n";
    out << "\tvec3 ld2 = normalize(lightPos2.xyz - (vec3(lightPos2.w) * oWp.xyz));\n";

    out << "\t// attenuation\n";
    out << "\tfloat lightDist = length(lightPos0.xyz - oWp.xyz) / (lightAtt0.x / lightAtt0.x);\n";
    out << "\tfloat la0 = 1.0;\n";
    out << "\tfloat ila = 0.0;\n";
    out << "\tif(lightAtt0.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\tila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla0 = 1.0 / (lightAtt0.g + lightAtt0.b * lightDist + lightAtt0.a * ila);\n";
    out << "\t}\n";

    out << "\t// attenuation\n";
    out << "\tlightDist = length(lightPos1.xyz - oWp.xyz) / (lightAtt1.x / lightAtt1.x);\n";
    out << "\tfloat la1 = 1.0;\n";
    out << "\tif(lightAtt1.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\tila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla1 = 1.0 / (lightAtt1.g + lightAtt1.b * lightDist + lightAtt1.a * ila);\n";
    out << "\t}\n";

    out << "\t// attenuation\n";
    out << "\tlightDist = length(lightPos2.xyz - oWp.xyz) / (lightAtt2.x / lightAtt2.x);\n";
    out << "\tfloat la2 = 1.0;\n";
    out << "\tif(lightAtt2.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\tila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla2 = 1.0 / (lightAtt2.g + lightAtt2.b * lightDist + lightAtt2.a * ila);\n";
    out << "\t}\n";

    out << "\tvec3 diffuse0 = vec3(max(dot(normal, ld0), 0.0)) * lightDif0;\n";
    out << "\tvec3 diffuse1 = vec3(max(dot(normal, ld1), 0.0)) * lightDif1;\n";
    out << "\tvec3 diffuse2 = vec3(max(dot(normal, ld2), 0.0)) * lightDif2;\n";

    ////saturate = clamp(val, 0, 1)
    out << "\t// calculate the spotlight effect\n";
    out << "\tfloat spot0 = (spotlightParams0.x == 1.0 && spotlightParams0.y == 0.0 && spotlightParams0.z == 0 && spotlightParams0.w == 1.0 ? 1.0 : // if so, then it's not a spot light\n";
    out << "\t   clamp(((dot(normalize(-oSpDir0), ld0) - spotlightParams0.y) / (spotlightParams0.x - spotlightParams0.y))), 0.0, 1.0);\n";
    out << "\tfloat spot1 = (spotlightParams1.x == 1.0 && spotlightParams1.y == 0.0 && spotlightParams1.z == 0.0 && spotlightParams1.w == 1.0 ? 1.0 : // if so, then it's not a spot light\n";
    out << "\t   clamp(((dot(normalize(-oSpDir1), ld1) - spotlightParams1.y) / (spotlightParams1.x - spotlightParams1.y))), 0.0, 1.0);\n";
    out << "\tfloat spot2 = (spotlightParams2.x == 1.0 && spotlightParams2.y == 0.0 && spotlightParams2.z == 0.0 && spotlightParams2.w == 1.0 ? 1.0 : // if so, then it's not a spot light\n";
    out << "\t   clamp(((dot(normalize(-oSpDir2), ld2) - spotlightParams2.y) / (spotlightParams2.x - spotlightParams2.y))), 0.0, 1.0);\n";

    out << "\tvec3 camDir = normalize(camPos - oWp.xyz);\n";
    out << "\tvec3 halfVec = normalize(ld0 + camDir);\n";
    out << "\tvec3 specularLight = pow(max(dot(normal, halfVec), 0), vec3(matShininess)) * lightSpec0;\n";
    out << "\thalfVec = normalize(ld1 + camDir);\n";
    out << "\tspecularLight += pow(max(dot(normal, halfVec), 0), vec3(matShininess)) * lightSpec1;\n";
    out << "\thalfVec = normalize(ld2 + camDir);\n";
    out << "\tspecularLight += pow(max(dot(normal, halfVec), 0), vec3(matShininess)) * lightSpec2;\n";

    out << "\tvec3 diffuseLight = (diffuse0 * vec3(spot0 * la0)) + (diffuse1 * vec3(spot1 * la1)) + (diffuse2 * vec3(spot2 * la2));\n";
    out << "\tvec3 ambientColor = max(matEmissive.xyz, ambient * matAmb.xyz);\n";

    out << "\tvec3 diffuseContrib = matDif.xyz;\n";

    if (bDiffuse)
    {
      out << "\tvec4 diffuseTex = texture2D(diffuseMap, uv";
      if (!(diffUv % 2))
        out << diffUv << ".xy" << ");\n";
      else
        out << diffUv - 1 << ".zw" << ");\n";

      out << "\tambientColor *= diffuseTex.xyz;\n";
      out << "\tdiffuseContrib *= diffuseTex.xyz;\n";
    }

    if (bAmbient)
    {
      out << "\tvec3 ambTex = texture2D(ambMap, uv";
      if (!(ambUv % 2))
        out << ambUv << ".xy" << ").xyz;\n";
      else
        out << ambUv - 1 << ".zw" << ").xyz;\n";
      out << "\tambientColor *= ambTex;\n";
    }

    if (bIllum)
    {
      out << "\tvec4 illTex = texture2D(illMap, uv";
      if (!(illUv % 2))
        out << illUv << ".xy" << ").xyz;\n";
      else
        out << illUv - 1 << ".zw" << ").xyz;\n";
      out << "\tambientColor = max(ambientColor, illTex.xyz);\n";
    }

    out << "\tvec3 specularContrib = specularLight * matSpec.xyz;\n";

    if (bSpecular)
    {
      out << "\tvec4 specTex = texture2D(specMap, uv";
      if (!(specUv % 2))
        out << specUv << ".xy" << ");\n";
      else
        out << specUv - 1 << ".zw" << ");\n";
      out << "\tspecularContrib *= specTex.xyz;\n";
    }

    //if(bAmbient)
    //out << "\tspecularContrib *= ambTex.xyz;\n";

    out << "\tvec3 light0C = ambientColor + (diffuseLight * diffuseContrib) + specularContrib;\n";

    out << "\tfloat alpha = matDif.a;\n";
    if (bDiffuse)
      out << "\talpha *= diffuseTex.a;\n";
    else if (bAmbient)
      out << "\talpha *= ambTex.a;\n";

    if (bRef)
    {
      out << "\tvec3 refVec = -reflect(camDir, normal);\n";
      out << "\trefVec.z = -refVec.z;\n";
      out << "\tvec4 reflecTex = texCUBE(reflectMap, refVec);\n";

      if (bFresnel)
      {
        out << "\tfloat fresnel = fresnelMul * reflectivity * pow(1.0 + dot(-camDir, normal), fresnelPow - (reflectivity * fresnelMul));\n";
        out << "\tvec4 reflecVal = reflecTex * fresnel;\n";
        out << "\tvec3 reflectColor = reflecVal.xyz * (1.0 - light0C);\n";
      }
      else //metal style
      {
        out << "\tvec3 reflectColor = reflecTex.xyz * reflectivity;\n";
      }

      out << "\treturn vec4(light0C + reflectColor, alpha);\n";
    }
    else
      out << "\treturn vec4(light0C, alpha);\n";

    out << "}\n";
    m_contentGles = out.str();
  }

  std::string& ExFpLightShader::getUniformParams(ExMaterial* mat)
  {
    std::stringstream out;
    out << "\t\t\tfragment_program_ref " << m_name << "\n";
		out << "\t\t\t{\n";
    
    if(bNormal)
    {
      out << "\t\t\t\tparam_named normalMul float " << mat->m_normalMul << "\n";
    }

    if(bRef)
    {
      out << "\t\t\t\tparam_named reflectivity float " << mat->m_reflectivity << "\n";
      if (bFresnel)
      {
        out << "\t\t\t\tparam_named fresnelMul float " << mat->m_reflectivity << "\n";
        out << "\t\t\t\tparam_named fresnelPow float " << (mat->m_softness * 4.0f) << "\n";
      }
    }

		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExFpLightShader::getProgram(std::string baseName)
  {
    std::stringstream out;

    //CG
    out << "fragment_program " << m_name << "_CG cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    if (bNeedHighProfile)
      out << "\tprofiles ps_3_x arbfp1\n";
    else
      out << "\tprofiles ps_3_0 arbfp1\n";
    out << "\tentry_point " << optimizeFileName(m_name) << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    
    out << "\t\tparam_named_auto ambient ambient_light_colour\n";
    out << "\t\tparam_named_auto lightDif0 light_diffuse_colour 0\n";
    out << "\t\tparam_named_auto lightPos0 light_position 0\n";
    out << "\t\tparam_named_auto lightAtt0 light_attenuation 0\n";
    out << "\t\tparam_named_auto lightSpec0 light_specular_colour 0\n";
    out << "\t\tparam_named_auto lightDif1 light_diffuse_colour 1\n";
    out << "\t\tparam_named_auto lightPos1 light_position 1\n";
    out << "\t\tparam_named_auto lightAtt1 light_attenuation 1\n";
    out << "\t\tparam_named_auto lightSpec1 light_specular_colour 1\n";
    out << "\t\tparam_named_auto lightDif2 light_diffuse_colour 2\n";
    out << "\t\tparam_named_auto lightPos2 light_position 2\n";
    out << "\t\tparam_named_auto lightAtt2 light_attenuation 2\n";
    out << "\t\tparam_named_auto lightSpec2 light_specular_colour 2\n";
    out << "\t\tparam_named_auto camPos camera_position\n";
    out << "\t\tparam_named_auto matAmb surface_ambient_colour\n";
    out << "\t\tparam_named_auto matEmissive surface_emissive_colour\n";
    out << "\t\tparam_named_auto matDif surface_diffuse_colour\n";
    out << "\t\tparam_named_auto matSpec surface_specular_colour\n";
    out << "\t\tparam_named_auto matShininess surface_shininess\n";
    out << "\t\tparam_named_auto spotlightParams0 spotlight_params 0\n";
    out << "\t\tparam_named_auto spotlightParams1 spotlight_params 1\n";
    out << "\t\tparam_named_auto spotlightParams2 spotlight_params 2\n";

    if(bNormal)
    {
      out << "\t\tparam_named_auto iTWMat inverse_transpose_world_matrix\n";
      out << "\t\tparam_named normalMul float 1\n";
    }

    if(bRef)
    {
      out << "\t\tparam_named reflectivity float 1.0\n";

      if (bFresnel)
      {
        out << "\t\tparam_named fresnelMul float 1.0\n";
        out << "\t\tparam_named fresnelPow float 1.0\n";
      }
    }

    out << "\t}\n";
    out << "}\n";

    //GLSLES
    out << "fragment_program " << m_name << "_GLSLES glsles\n";
    out << "{\n";

    out << "\tsource " << optimizeFileName(m_name) << "FP.glsles\n";
    out << "\tprofiles glsles\n";

    out << "\tdefault_params\n";
    out << "\t{\n";

    out << "\t\tparam_named_auto ambient ambient_light_colour\n";
    out << "\t\tparam_named_auto lightDif0 light_diffuse_colour 0\n";
    out << "\t\tparam_named_auto lightPos0 light_position 0\n";
    out << "\t\tparam_named_auto lightAtt0 light_attenuation 0\n";
    out << "\t\tparam_named_auto lightSpec0 light_specular_colour 0\n";
    out << "\t\tparam_named_auto lightDif1 light_diffuse_colour 1\n";
    out << "\t\tparam_named_auto lightPos1 light_position 1\n";
    out << "\t\tparam_named_auto lightAtt1 light_attenuation 1\n";
    out << "\t\tparam_named_auto lightSpec1 light_specular_colour 1\n";
    out << "\t\tparam_named_auto lightDif2 light_diffuse_colour 2\n";
    out << "\t\tparam_named_auto lightPos2 light_position 2\n";
    out << "\t\tparam_named_auto lightAtt2 light_attenuation 2\n";
    out << "\t\tparam_named_auto lightSpec2 light_specular_colour 2\n";
    out << "\t\tparam_named_auto camPos camera_position\n";
    out << "\t\tparam_named_auto matAmb surface_ambient_colour\n";
    out << "\t\tparam_named_auto matEmissive surface_emissive_colour\n";
    out << "\t\tparam_named_auto matDif surface_diffuse_colour\n";
    out << "\t\tparam_named_auto matSpec surface_specular_colour\n";
    out << "\t\tparam_named_auto matShininess surface_shininess\n";
    out << "\t\tparam_named_auto spotlightParams0 spotlight_params 0\n";
    out << "\t\tparam_named_auto spotlightParams1 spotlight_params 1\n";
    out << "\t\tparam_named_auto spotlightParams2 spotlight_params 2\n";

    if (bNormal)
    {
      out << "\t\tparam_named_auto iTWMat inverse_transpose_world_matrix\n";
      out << "\t\tparam_named normalMul float 1\n";
    }

    if (bRef)
    {
      out << "\t\tparam_named reflectivity float 1.0\n";

      if (bFresnel)
      {
        out << "\t\tparam_named fresnelMul float 1.0\n";
        out << "\t\tparam_named fresnelPow float 1.0\n";
      }
    }

    out << "\t}\n";
    out << "}\n";

    //DELEGATE
    out << "fragment_program " << m_name << " unified\n";
    out << "{\n";

    out << "\tdelegate " << m_name << "_CG\n";
    out << "\tdelegate " << m_name << "_GLSLES\n";

    out << "}\n";

    m_program = out.str();
    return m_program;
  }

// ExVsLightShaderMulti
ExVsLightShaderMulti::ExVsLightShaderMulti(std::string name) : ExShader(name)
{
  m_type = ST_VSLIGHT_MULTI;
}

ExVsLightShaderMulti::~ExVsLightShaderMulti()
{
}

void ExVsLightShaderMulti::constructShader(ExMaterial* mat)
{
  // get the material config
  bFresnel = (mat->m_type == MT_METAL) ? false : true;
  bRef = mat->m_hasReflectionMap;
  bNormal = mat->m_hasBumpMap;
  bDiffuse = mat->m_hasDiffuseMap;
  bSpecular = mat->m_hasSpecularMap;

  std::vector<int> texUnits;
  for (int i = 0; i < mat->m_textures.size(); i++)
  {
    if (mat->m_textures[i].bCreateTextureUnit == true)
      texUnits.push_back(mat->m_textures[i].uvsetIndex);
  }

  std::sort(texUnits.begin(), texUnits.end());
  texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
  std::stringstream out;

  // generate the shader
  out << "void " << optimizeFileName(m_name).c_str() << "(float4 position	: POSITION,\n";
  out << "\tfloat3 normal : NORMAL,\n";
  if (bNormal)
  {
    out << "\tfloat3 tangent : TANGENT,\n";
  }

  for (int i = 0; i < texUnits.size(); i++)
  {
    out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texUnits[i] << ",\n";
  }

  int texCoord = 0;
  out << "\tout float4 oPos : POSITION,\n";
  out << "\tout float3 oNorm : TEXCOORD" << texCoord++ << ",\n";

  if (bNormal)
  {
    out << "\tout float3 oTang : TEXCOORD" << texCoord++ << ",\n";
    out << "\tout float3 oBinormal : TEXCOORD" << texCoord++ << ",\n";
  }

  out << "\tout float3 oSpDir : TEXCOORD" << texCoord++ << ",\n";
  out << "\tout float4 oWp : TEXCOORD" << texCoord++ << ",\n";

  int lastAvailableTexCoord = texCoord;
  for (int i = 0; i < texUnits.size() && (texCoord < 16); i++)
  {
    out << "\tout float2 oUv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
  }

  if (texCoord > 8)
    bNeedHighProfile = true;

  out << "\tuniform float4x4 wMat,\n";

  if (!bNormal)
    out << "\tuniform float4x4 witMat,\n";

  out << "\tuniform float4x4 wvpMat,\n";
  out << "\tuniform float4 spotlightDir";

  out << ")\n";
  out << "{\n";

  out << "\toWp = mul(wMat, position);\n";
  out << "\toPos = mul(wvpMat, position);\n";

  texCoord = lastAvailableTexCoord;
  for (int i = 0; i < texUnits.size() && (texCoord < 16); i++)
  {
    out << "\toUv" << texUnits[i] << " = uv" << texUnits[i] << ";\n";
    texCoord++;
  }

  if (texCoord > 8)
    bNeedHighProfile = true;

  if (bNormal)
  {
    out << "\toTang = tangent;\n";
    out << "\toBinormal = cross(tangent, normal);\n";
    out << "\toNorm = normal;\n";
  }
  else
  {
    out << "\toNorm = normalize(mul((float3x3)witMat, normal));\n";
  }

  out << "\toSpDir = mul(wMat, spotlightDir).xyz;\n";
  out << "}\n";
  m_content = out.str();
}

std::string& ExVsLightShaderMulti::getUniformParams(ExMaterial* mat)
{
  std::stringstream out;
  out << "\t\t\tvertex_program_ref " << m_name << "\n";
  out << "\t\t\t{\n";
  out << "\t\t\t}\n";
  m_params = out.str();
  return m_params;
}

std::string& ExVsLightShaderMulti::getProgram(std::string baseName)
{
  std::stringstream out;
  out << "vertex_program " << m_name << " cg\n";
  out << "{\n";

  out << "\tsource " << baseName << ".cg\n";
  if (bNeedHighProfile)
    out << "\tprofiles vs_4_0 arbvp2\n";
  else
    out << "\tprofiles vs_1_1 arbvp1\n";
  out << "\tentry_point " << optimizeFileName(m_name) << "\n";

  out << "\tdefault_params\n";
  out << "\t{\n";
  out << "\t\tparam_named_auto wMat world_matrix\n";

  if (!bNormal)
    out << "\t\tparam_named_auto witMat inverse_transpose_world_matrix\n";

  out << "\t\tparam_named_auto wvpMat worldviewproj_matrix\n";
  out << "\t\tparam_named_auto spotlightDir light_direction_object_space 0\n";

  out << "\t}\n";
  out << "}\n";

  m_program = out.str();
  return m_program;
}


// ExFpLightShaderMulti
ExFpLightShaderMulti::ExFpLightShaderMulti(std::string name) : ExShader(name)
{
  m_type = ST_FPLIGHT_MULTI;
}

ExFpLightShaderMulti::~ExFpLightShaderMulti()
{
}

void ExFpLightShaderMulti::constructShader(ExMaterial* mat)
{
  // get the material config
  bFresnel = (mat->m_type == MT_METAL) ? false : true;
  bRef = mat->m_hasReflectionMap;
  bNormal = mat->m_hasBumpMap;
  bDiffuse = mat->m_hasDiffuseMap;
  bSpecular = mat->m_hasSpecularMap;
  bAmbient = mat->m_hasAmbientMap;

  std::vector<int> texUnits;
  for (int i = 0; i < mat->m_textures.size(); i++)
  {
    if (mat->m_textures[i].bCreateTextureUnit == true)
    {
      texUnits.push_back(mat->m_textures[i].uvsetIndex);
    }
  }

  std::sort(texUnits.begin(), texUnits.end());
  texUnits.erase(std::unique(texUnits.begin(), texUnits.end()), texUnits.end());
  std::stringstream out;

  int texCoord = 0;
  // generate the shader
  out << "float4 " << optimizeFileName(m_name).c_str() << "(float4 position	: POSITION,\n";
  out << "\tfloat3 norm : TEXCOORD" << texCoord++ << ",\n";

  if (bNormal)
  {
    out << "\tfloat3 tangent : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat3 binormal : TEXCOORD" << texCoord++ << ",\n";
  }

  out << "\tfloat3 spDir : TEXCOORD" << texCoord++ << ",\n";
  out << "\tfloat4 wp : TEXCOORD" << texCoord++ << ",\n";

  for (int i = 0; i < texUnits.size() && (texCoord < 16); i++)
  {
    out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
  }

  if (texCoord > 8)
    bNeedHighProfile = true;

  if (bAmbient)
    out << "\tuniform float3 ambient,\n";

  out << "\tuniform float3 lightDif0,\n";
  out << "\tuniform float4 lightPos0,\n";
  out << "\tuniform float4 lightAtt0,\n";
  out << "\tuniform float3 lightSpec0,\n";
  out << "\tuniform float4 matDif,\n";
  out << "\tuniform float4 matSpec,\n";
  out << "\tuniform float matShininess,\n";
  out << "\tuniform float3 camPos,\n";
  out << "\tuniform float4 invSMSize,\n";
  out << "\tuniform float4 spotlightParams";
  if (bNormal)
  {
    out << ",\n\tuniform float4x4 iTWMat";
    out << ",\n\tuniform float normalMul";
  }

  if (bRef)
  {
    out << ",\n\tuniform float reflectivity";

    if (bFresnel)
    {
      out << ",\n\tuniform float fresnelMul";
      out << ",\n\tuniform float fresnelPow";
    }
  }

  int samplerId = 0;
  int ambUv = 0;
  int diffUv = 0;
  int specUv = 0;
  int normUv = 0;

  for (int i = 0; i < mat->m_textures.size(); i++)
  {
    if (mat->m_textures[i].bCreateTextureUnit == true)
    {
      switch (mat->m_textures[i].type)
      {
        //could be a light map
      case ID_AM:
        out << ",\n\tuniform sampler2D ambMap : register(s" << samplerId << ")";
        ambUv = mat->m_textures[i].uvsetIndex;
        samplerId++;
        break;

      case ID_DI:
        out << ",\n\tuniform sampler2D diffuseMap : register(s" << samplerId << ")";
        diffUv = mat->m_textures[i].uvsetIndex;
        samplerId++;
        break;

      case ID_SP:
        out << ",\n\tuniform sampler2D specMap : register(s" << samplerId << ")";
        specUv = mat->m_textures[i].uvsetIndex;
        samplerId++;
        break;

      case ID_BU:
        out << ",\n\tuniform sampler2D normalMap : register(s" << samplerId << ")";
        normUv = mat->m_textures[i].uvsetIndex;
        samplerId++;
        break;

      case ID_RL:
        out << ",\n\tuniform samplerCUBE reflectMap : register(s" << samplerId << ")";
        samplerId++;
        break;
      }
    }
  }

  out << "): COLOR0\n";
  out << "{\n";

  // direction
  out << "\tfloat3 ld0 = normalize(lightPos0.xyz - (lightPos0.w * wp.xyz));\n";

  out << "\t// attenuation\n";
  out << "\thalf lightDist = length(lightPos0.xyz - wp.xyz) / (lightAtt0.r / lightAtt0.r);\n";
  out << "\thalf la = 1;\n";
  out << "\tif(lightAtt0.a > 0.0)\n";
  out << "\t{\n";
  out << "\t\thalf ila = lightDist * lightDist; // quadratic falloff\n";
  out << "\t\tla = 1.0 / (lightAtt0.g + lightAtt0.b * lightDist + lightAtt0.a * ila);\n";
  out << "\t}\n";

  if (bNormal)
  {
    out << "\tfloat3 normalTex = tex2D(normalMap, uv" << normUv << ");\n";
    out << "\ttangent *= normalMul;\n";
    out << "\tbinormal *= normalMul;\n";
    out << "\tfloat3x3 tbn = float3x3(tangent, binormal, norm);\n";
    out << "\tfloat3 normal = mul(transpose(tbn), (normalTex.xyz -0.5) * 2); // to object space\n";
    out << "\tnormal = normalize(mul((float3x3)iTWMat, normal));\n";
  }
  else
  {
    out << "\tfloat3 normal = normalize(norm);\n";
  }

  out << "\tfloat3 diffuse = max(dot(normal, ld0), 0);\n";

  out << "\t// calculate the spotlight effect\n";
  out << "\tfloat spot = (spotlightParams.x == 1 && spotlightParams.y == 0 && spotlightParams.z == 0 && spotlightParams.w == 1 ? 1 : // if so, then it's not a spot light\n";
  out << "\t   saturate((dot(normalize(-spDir), ld0) - spotlightParams.y) / (spotlightParams.x - spotlightParams.y)));\n";

  out << "\tfloat3 camDir = normalize(camPos - wp.xyz);\n";
  out << "\tfloat3 halfVec = normalize(ld0 + camDir);\n";
  out << "\tfloat3 specular = pow(max(dot(normal, halfVec), 0), matShininess);\n";

  if (bAmbient)
    out << "\tfloat4 ambTex = tex2D(ambMap, uv" << ambUv << ");\n";

  if (bDiffuse)
    out << "\tfloat4 diffuseTex = tex2D(diffuseMap, uv" << diffUv << ");\n";

  if (bSpecular)
    out << "\tfloat4 specTex = tex2D(specMap, uv" << specUv << ");\n";

  out << "\tfloat3 diffuseContrib = diffuse * lightDif0 * matDif.rgb;\n";

  if (bAmbient)
    out << "\tdiffuseContrib += ambTex.rgb * ambient;\n";
  //out << "\tdiffuseContrib *= ambTex.rgb;\n";

  if (bDiffuse)
    out << "\tdiffuseContrib *= diffuseTex.rgb;\n";

  out << "\tfloat3 specularContrib = specular * lightSpec0 * matSpec.rgb;\n";

  if (bSpecular)
    out << "\tspecularContrib *= specTex.rgb;\n";

  if (bAmbient)
    out << "\tspecularContrib *= ambTex.rgb;\n";

  out << "\tfloat3 light0C = (diffuseContrib + specularContrib) * la * spot;\n";

  out << "\tfloat alpha = matDif.a;\n";
  if (bDiffuse)
    out << "\talpha *= diffuseTex.a;\n";
  else if (bAmbient)
    out << "\talpha *= ambTex.a;\n";

  if (bRef)
  {
    out << "\tfloat3 refVec = -reflect(camDir, normal);\n";
    out << "\trefVec.z = -refVec.z;\n";
    out << "\tfloat4 reflecTex = texCUBE(reflectMap, refVec);\n";

    if (bFresnel)
    {
      out << "\tfloat fresnel = fresnelMul * reflectivity * pow(1 + dot(-camDir, normal), fresnelPow - (reflectivity * fresnelMul));\n";
      out << "\tfloat4 reflecVal = reflecTex * fresnel;\n";
      out << "\tfloat3 reflectColor = reflecVal.rgb * (1.0f - diffuseContrib) + (reflecVal.rgb * specularContrib);\n";
    }
    else
    {
      out << "\tfloat4 reflecVal = reflecTex * reflectivity;\n";
      out << "\tfloat3 reflectColor = reflecVal.rgb * (1.0f - diffuseContrib) + (reflecVal.rgb * specularContrib);\n";
    }

    out << "\treturn float4(light0C + reflectColor, alpha);\n";
  }
  else
  {
    out << "\treturn float4(light0C, alpha);\n";
  }

  out << "}\n";
  m_content = out.str();
}

std::string& ExFpLightShaderMulti::getUniformParams(ExMaterial* mat)
{
  std::stringstream out;
  out << "\t\t\tfragment_program_ref " << m_name << "\n";
  out << "\t\t\t{\n";

  if (bNormal)
  {
    out << "\t\t\t\tparam_named normalMul float " << mat->m_normalMul << "\n";
  }

  if (bRef)
  {
    out << "\t\t\t\tparam_named reflectivity float " << mat->m_reflectivity << "\n";

    if (bFresnel)
    {
      out << "\t\t\t\tparam_named fresnelMul float " << mat->m_reflectivity << "\n";
      out << "\t\t\t\tparam_named fresnelPow float " << (mat->m_softness * 4.0f) << "\n";
    }
  }

  out << "\t\t\t}\n";
  m_params = out.str();
  return m_params;
}

std::string& ExFpLightShaderMulti::getProgram(std::string baseName)
{
  std::stringstream out;
  out << "fragment_program " << m_name << " cg\n";
  out << "{\n";

  out << "\tsource " << baseName << ".cg\n";
  if (bNeedHighProfile)
    out << "\tprofiles ps_3_x arbfp1\n";
  else
    out << "\tprofiles ps_3_0 arbfp1\n";
  out << "\tentry_point " << optimizeFileName(m_name) << "\n";

  out << "\tdefault_params\n";
  out << "\t{\n";

  if (bAmbient)
    out << "\t\tparam_named_auto ambient ambient_light_colour\n";

  out << "\t\tparam_named_auto lightDif0 light_diffuse_colour 0\n";
  out << "\t\tparam_named_auto lightSpec0 light_specular_colour 0\n";
  out << "\t\tparam_named_auto camPos camera_position\n";
  out << "\t\tparam_named_auto matShininess surface_shininess\n";
  out << "\t\tparam_named_auto matDif surface_diffuse_colour\n";
  out << "\t\tparam_named_auto matSpec surface_specular_colour\n";
  out << "\t\tparam_named_auto lightPos0 light_position 0\n";
  out << "\t\tparam_named_auto lightAtt0 light_attenuation 0\n";
  out << "\t\tparam_named_auto spotlightParams spotlight_params 0\n";

  if (bNormal)
  {
    out << "\t\tparam_named_auto iTWMat inverse_transpose_world_matrix\n";
    out << "\t\tparam_named normalMul float 1\n";
  }

  if (bRef)
  {
    out << "\t\tparam_named reflectivity float 1.0\n";

    if (bFresnel)
    {
      out << "\t\tparam_named fresnelMul float 1.0\n";
      out << "\t\tparam_named fresnelPow float 1.0\n";
    }
  }

  out << "\t}\n";
  out << "}\n";

  m_program = out.str();
  return m_program;
}

};