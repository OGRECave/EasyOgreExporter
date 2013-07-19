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
  }

	ExShader::~ExShader()
	{
	}

  void ExShader::constructShader(ExMaterial* mat)
  {
  }

	std::string& ExShader::getName()
	{
		return m_name;
	}

	std::string& ExShader::getContent()
	{
		return m_content;
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
    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\tout float2 oUv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

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
    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\toUv" << texUnits[i] << " = uv" << texUnits[i] << ";\n";
      texCoord++;
    }

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

    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tuniform float3 ambient,\n";
    out << "\tuniform float4 matAmb,\n";
    out << "\tuniform float4 matEmissive";

    if(bRef)
    {
      out << ",\n\tuniform float3 camPos";
      out << ",\n\tuniform float reflectivity";
      out << ",\n\tuniform float fresnelMul";
      out << ",\n\tuniform float fresnelPow";
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
      out << "\tfloat fresnel = fresnelMul * reflectivity * pow(1 + dot(-camDir, normal), fresnelPow - (reflectivity * fresnelMul));\n";
      out << "\tfloat4 reflecVal = reflecTex * fresnel;\n";
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
      out << "\t\t\t\tparam_named fresnelMul float 4.0\n";
      out << "\t\t\t\tparam_named fresnelPow float 5.0\n";
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
    out << "\tprofiles ps_2_x arbfp1\n";
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
      out << "\t\tparam_named fresnelMul float 4.0\n";
      out << "\t\tparam_named fresnelPow float 5.0\n";
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

    out << "\tout float3 oSpDir : TEXCOORD" << texCoord++ << ",\n";
    out << "\tout float4 oWp : TEXCOORD" << texCoord++ << ",\n";

    int lastAvailableTexCoord = texCoord;
    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\tout float2 oUv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

    out << "\tuniform float4x4 wMat,\n";
    out << "\tuniform float4x4 wvpMat,\n";
    out << "\tuniform float4 spotlightDir";
    
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
    out << "}\n";
    m_content = out.str();
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
    out << "vertex_program " << m_name << " cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    out << "\tprofiles vs_1_1 arbvp1\n";
    out << "\tentry_point " << optimizeFileName(m_name) << "\n";

    out << "\tdefault_params\n";
    out << "\t{\n";
    out << "\t\tparam_named_auto wMat world_matrix\n";
    out << "\t\tparam_named_auto wvpMat worldviewproj_matrix\n";
    out << "\t\tparam_named_auto spotlightDir light_direction_object_space 0\n";

    out << "\t}\n";
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
    bRef = mat->m_hasReflectionMap;
    bNormal = mat->m_hasBumpMap;
    bDiffuse = mat->m_hasDiffuseMap;
    bSpecular = mat->m_hasSpecularMap;
    bAmbient = mat->m_hasAmbientMap;

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

    out << "\tfloat3 spDir : TEXCOORD" << texCoord++ << ",\n";
    out << "\tfloat4 wp : TEXCOORD" << texCoord++ << ",\n";

    for (int i=0; i < texUnits.size() && (texCoord < 8); i++)
    {
      out << "\tfloat2 uv" << texUnits[i] << " : TEXCOORD" << texCoord++ << ",\n";
    }

    if (bAmbient)
      out << "\t\tuniform float3 ambient,\n";

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
    {
      out << ",\n\tuniform float4x4 iTWMat";
      out << ",\n\tuniform float normalMul";
    }

    if(bRef)
    {
      out << ",\n\tuniform float reflectivity";
      out << ",\n\tuniform float fresnelMul";
      out << ",\n\tuniform float fresnelPow";
    }

    int samplerId = 0;
    int ambUv = 0;
    int diffUv = 0;
    int specUv = 0;
    int normUv = 0;

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
    out << "\thalf lightDist = length(lightPos0.xyz - wp.xyz) / lightAtt0.r;\n";
    out << "\thalf la = 1;\n";
    out << "\tif(lightAtt0.a > 0.0)\n";
    out << "\t{\n";
    out << "\t\thalf ila = lightDist * lightDist; // quadratic falloff\n";
    out << "\t\tla = 1.0 / (lightAtt0.g + lightAtt0.b * lightDist + lightAtt0.a * ila);\n";
    out << "\t}\n";

    if(bNormal)
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

    out << "\tfloat3 diffuse = max(dot(ld0, normal), 0);\n";

    out << "\t// calculate the spotlight effect\n";
    out << "\tfloat spot = (spotlightParams.x == 1 && spotlightParams.y == 0 && spotlightParams.z == 0 && spotlightParams.w == 1 ? 1 : // if so, then it's not a spot light\n";
    out << "\t   saturate((dot(ld0, normalize(-spDir)) - spotlightParams.y) / (spotlightParams.x - spotlightParams.y)));\n";

    out << "\tfloat3 camDir = normalize(camPos - wp.xyz);\n";
    out << "\tfloat3 halfVec = normalize(ld0 + camDir);\n";
    out << "\tfloat3 specular = pow(max(dot(normal, halfVec), 0), matShininess);\n";

    if(bAmbient)
      out << "\tfloat4 ambTex = tex2D(ambMap, uv" << ambUv << ");\n";

    if(bDiffuse)
      out << "\tfloat4 diffuseTex = tex2D(diffuseMap, uv" << diffUv << ");\n";
    
    if(bSpecular)
      out << "\tfloat4 specTex = tex2D(specMap, uv" << specUv << ");\n";

    out << "\tfloat3 diffuseContrib = diffuse * lightDif0 * matDif.rgb;\n";
    
    if(bAmbient)
      out << "\tdiffuseContrib += ambTex.rgb * ambient;\n";
      //out << "\tdiffuseContrib *= ambTex.rgb;\n";

    if(bDiffuse)
      out << "\tdiffuseContrib *= diffuseTex.rgb;\n";

    out << "\tfloat3 specularContrib = specular * lightSpec0 * matSpec.rgb;\n";

    if(bSpecular)
      out << "\tspecularContrib *= specTex.rgb;\n";

    if(bAmbient)
      out << "\tspecularContrib *= ambTex.rgb;\n";

    out << "\tfloat3 light0C = (diffuseContrib + specularContrib) * la * spot;\n";
    
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
      out << "\tfloat fresnel = fresnelMul * reflectivity * pow(1 + dot(-camDir, normal), fresnelPow - (reflectivity * fresnelMul));\n";
      out << "\tfloat4 reflecVal = reflecTex * fresnel;\n";

      out << "\tfloat3 reflectColor = (reflecVal.rgb * diffuseContrib) + (reflecVal.rbg * specularContrib);\n";
      out << "\treturn float4(light0C + reflectColor, alpha);\n";
    }
    else
    {
      out << "\treturn float4(light0C, alpha);\n";
    }

    out << "}\n";
    m_content = out.str();
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
      out << "\t\t\t\tparam_named fresnelMul float 4.0\n";
      out << "\t\t\t\tparam_named fresnelPow float 5.0\n";
    }

		out << "\t\t\t}\n";
    m_params = out.str();
    return m_params;
  }

  std::string& ExFpLightShader::getProgram(std::string baseName)
  {
    std::stringstream out;
    out << "fragment_program " << m_name << " cg\n";
    out << "{\n";
    
    out << "\tsource " << baseName << ".cg\n";
    out << "\tprofiles ps_2_x arbfp1\n";
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
    
    if(bNormal)
    {
      out << "\t\tparam_named_auto iTWMat inverse_transpose_world_matrix\n";
      out << "\t\tparam_named normalMul float 1\n";
    }

    if(bRef)
    {
      out << "\t\tparam_named reflectivity float 1.0\n";
      out << "\t\tparam_named fresnelMul float 4.0\n";
      out << "\t\tparam_named fresnelPow float 5.0\n";
    }

    out << "\t}\n";
    out << "}\n";

    m_program = out.str();
    return m_program;
  }
};