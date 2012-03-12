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
  // Constructor
  ExVsShader::ExVsShader(std::string name, ExMaterial* mat)
	{
    m_name = name;
    
    // get the material config
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
    out << "void " << m_name.c_str() << "(float4 position	: POSITION,\n"; 
    out << "\tfloat3 normal		: NORMAL,\n";
    if(bNormal)
    {
      out << "\tfloat3 tangent	: TANGENT,\n";
    }

    for (int i=0; i < texUnits.size(); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texUnits[i] << ",\n";
    }

    int texCoord = 0;
    out << "\tout float4 oPos: POSITION,\n";
    out << "\tout float3 oNorm: TEXCOORD" << texCoord++ << ",\n";

    if(bNormal)
    {
      out << "\tout float3 oTang: TEXCOORD" << texCoord++ << ",\n";
      out << "\tout float3 oBinormal: TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tout float3 oSpDir: TEXCOORD" << texCoord++ << ",\n";
    out << "\tout float4 oWp: TEXCOORD" << texCoord++ << ",\n";
    if(bRef)
      out << "\tout float3 otexProj: TEXCOORD" << texCoord++ << ",\n";

    int lastAvailableTexCoord = texCoord;
    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\tout float2 oUv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tuniform float4x4 wMat,\n";
    out << "\tuniform float4x4 wvpMat,\n";
    out << "\tuniform float4 spotlightDir";

    if(bRef)
      out << ",\n\tuniform float3 eyePositionW";
    
    out << ")\n";
    out << "{\n";

    out << "\toWp = mul(wMat, position);\n";
    out << "\toPos = mul(wvpMat, position);\n";
    out << "\toNorm = normal;\n";


    texCoord = lastAvailableTexCoord;
    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\toUv" << texUnits[i] << " = uv" << texUnits[i] << ";\n";
      texCoord++;
    }

    if(bNormal)
    {
      out << "\toTang = tangent;\n";
      out << "\toBinormal = cross(tangent, normal);\n";
    }

    out << "\toSpDir = mul(wMat, spotlightDir).xyz;\n";

    if(bRef)
    {
      out << "\tfloat3 posW = mul(wMat, IN.p).xyz;\n";
      out << "\tfloat3 refNorm = normalize(mul((float3x3)wMat, normal));\n";
      out << "\tfloat3 eyePos = oWp.xyz - eyePositionW;\n";
      
      out << "\totexProj = reflect(eyePos, refNorm);\n";
      out << "\totexProj.z = - otexProj.z;\n";
    }
    out << "}\n";
    m_content = out.str();
	}

	// Destructor
	ExVsShader::~ExVsShader()
	{
	}

	std::string& ExVsShader::getName()
	{
		return m_name;
	}

	std::string& ExVsShader::getContent()
	{
		return m_content;
	}

  std::string& ExVsShader::getUniformParams()
  {
    std::stringstream out;
    out << "\t\t\tvertex_program_ref " << m_name << "\n";
		out << "\t\t\t{\n";
		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExVsShader::getProgram(std::string baseName)
  {
    std::stringstream out;
    out << "vertex_program " << m_name << " cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    out << "\tprofiles vs_1_1 arbvp1\n";
    out << "\tentry_point " << m_name << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto wMat world_matrix\n";
    out << "\t\tparam_named_auto wvpMat worldviewproj_matrix\n";
    out << "\t\tparam_named_auto spotlightDir light_direction_object_space 0\n";
    if(bRef)
      out << "\t\tparam_named_auto eyePositionW camera_position\n";

    out << "\t}\n";
    out << "}\n";

    m_program = out.str();
    return m_program;
  }

  // Constructor
  ExFpShader::ExFpShader(std::string name, ExMaterial* mat)
	{
    m_name = name;

    // get the material config
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;

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
    out << "float4 " << m_name.c_str() << "(float4 position	: POSITION,\n";
    out << "\tfloat3 norm	    " << "    : TEXCOORD" << texCoord++ << ",\n";

    if(bNormal)
    {
      out << "\tfloat3 tangent  " << "    : TEXCOORD" << texCoord++ << ",\n";
      out << "\tfloat3 binormal " << "    : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tfloat3 spDir          : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat4 wp             : TEXCOORD" << texCoord++ << ",\n";
    if(bRef)
      out << "\tfloat3 texProj      : TEXCOORD" << texCoord++ << ",\n";

    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << "       : TEXCOORD" << texCoord++ << ",\n";
    }

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
    if(bNormal)
      out << ",\n\tuniform float4x4 iTWMat";
    
    if(bRef)
      out << ",\n\tuniform float reflectivity";

    int samplerId = 0;
    int diffUv = 0;
    int specUv = 0;
    int normUv = 0;

    for (int i=0; i < mat->m_textures.size(); i++)
    {
	    if(mat->m_textures[i].bCreateTextureUnit == true)
	    {
        switch (mat->m_textures[i].type)
        {
          case ID_AM:
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

    out << "\thalf lightDist = length(lightPos0.xyz - wp.xyz) / lightAtt0.r;\n";
    out << "\t// attenuation\n";
    out << "\thalf ila = lightDist * lightDist; // quadratic falloff\n";
    out << "\thalf la = 1.0 - ila;\n";

    if(bNormal)
    {
      out << "\tfloat4 normalTex = tex2D(normalMap, uv" << normUv << ");\n";
      out << "\tfloat3x3 tbn = float3x3(tangent, binormal, norm);\n";
      out << "\tfloat3 normal = mul(transpose(tbn), normalTex.xyz * 2 - 1); // to object space\n";
      out << "\tnormal = normalize(mul((float3x3)iTWMat, normal));\n";
    }
    else
    {
      out << "\tfloat3 normal = normalize(norm);\n";
    }

    out << "\tfloat3 diffuse = max(dot(ld0, normal), 0);\n";

    out << "\t// calculate the spotlight effect\n";
    out << "\tfloat spot = (spotlightParams.x == 1 && spotlightParams.y == 0 && spotlightParams.z == 0 && spotlightParams.w == 1 ? 1 : // if so, then it's not a spot light\n";
    out << "\t   saturate((dot(ld0, normalize(-spDir)) - spotlightParams.y) / (spotlightParams.x - spotlightParams.y)));\n";

    out << "\tfloat3 camDir = normalize(camPos - wp.xyz);\n";
    out << "\tfloat3 halfVec = normalize(ld0 + camDir);\n";
    out << "\tfloat3 specular = pow(max(dot(normal, halfVec), 0), matShininess);\n";

    if(bDiffuse)
      out << "\tfloat4 diffuseTex = tex2D(diffuseMap, uv" << diffUv << ");\n";
    
    if(bSpecular)
      out << "\tfloat4 specTex = tex2D(specMap, uv" << specUv << ");\n";
    
    if(bRef)
      out << "\tfloat4 reflecTex = texCUBE(reflectMap, texProj);\n";

    if(bDiffuse)
      out << "\tfloat3 diffuseContrib = (diffuse * lightDif0 * diffuseTex.rgb * matDif.rgb);\n";
    else
      out << "\tfloat3 diffuseContrib = (diffuse * lightDif0 * matDif.rgb);\n";

    if(bSpecular)
      out << "\tfloat3 specularContrib = (specular * lightSpec0 * specTex.rgb * matSpec.rgb);\n";
    else
      out << "\tfloat3 specularContrib = (specular * lightSpec0 * matSpec.rgb);\n";

    out << "\tfloat3 light0C = (diffuseContrib + specularContrib) * la * spot;\n";
    
    if(bRef)
    {
      out << "\tfloat3 reflectColor = (reflecTex.rgb * diffuseContrib * reflectivity) + (reflecTex.rbg * specularContrib * reflectivity);\n";
      out << "\treturn float4(light0C + reflectColor, " << (mat->m_hasDiffuseMap ? "diffuseTex.a" : "1.0") <<");\n";
    }
    else
    {
      out << "\treturn float4(light0C, " << (mat->m_hasDiffuseMap ? "diffuseTex.a" : "1.0") <<");\n";
    }

    out << "}\n";
    m_content = out.str();
	}

	// Destructor
	ExFpShader::~ExFpShader()
	{
	}

	std::string& ExFpShader::getName()
	{
		return m_name;
	}

	std::string& ExFpShader::getContent()
	{
		return m_content;
	}

  std::string& ExFpShader::getUniformParams()
  {
    std::stringstream out;
    out << "\t\t\tfragment_program_ref " << m_name << "\n";
		out << "\t\t\t{\n";
		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExFpShader::getProgram(std::string baseName)
  {
    std::stringstream out;
    out << "fragment_program " << m_name << " cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    out << "\tprofiles ps_2_x arbfp1\n";
    out << "\tentry_point " << m_name << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto lightDif0 light_diffuse_colour 0\n";
    out << "\t\tparam_named_auto lightSpec0 light_specular_colour 0\n";
    out << "\t\tparam_named_auto camPos camera_position\n";
    out << "\t\tparam_named_auto matShininess surface_shininess\n";
    out << "\t\tparam_named_auto matDif surface_diffuse_colour\n";
    out << "\t\tparam_named_auto matSpec surface_specular_colour\n";
    out << "\t\tparam_named_auto lightPos0 light_position 0\n";
    out << "\t\tparam_named_auto lightAtt0 light_attenuation 0\n";
    out << "\t\tparam_named_auto spotlightParams spotlight_params 0\n";
    
    if(bNormal)
      out << "\t\tparam_named_auto iTWMat inverse_transpose_world_matrix\n";

    if(bRef)
      out << "\t\tparam_named reflectivity float 0\n";

    out << "\t}\n";
    out << "}\n";

    m_program = out.str();
    return m_program;
  }
};