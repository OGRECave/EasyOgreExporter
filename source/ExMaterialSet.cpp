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
#include <nvtt/nvtt.h>

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

  ExShader* ExMaterialSet::createShader(ExMaterial* mat, ExShader::ShaderType type, ParamList &params)
  {
    std::string sname = mat->getShaderName(type, params.resPrefix);
    ExShader* shader = getShader(sname);
    if(shader)
      return shader;
    
    switch (type)
    {
      case ExShader::ST_VSAM:
        shader = new ExVsAmbShader(sname);
      break;

      case ExShader::ST_FPAM:
        shader = new ExFpAmbShader(sname);
      break;

      case ExShader::ST_VSLIGHT:
        shader = new ExVsLightShader(sname);
      break;

      case ExShader::ST_FPLIGHT:
        shader = new ExFpLightShader(sname);
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
      ExShader* vsAmbShader = 0;
      ExShader* fpAmbShader = 0;
      ExShader* vsLightShader = 0;
      ExShader* fpLightShader = 0;

      if((params.exportProgram == SHADER_ALL) || ((params.exportProgram == SHADER_BUMP) && ((m_materials[i]->m_hasBumpMap) || (m_materials[i]->m_hasSpecularMap))))
      {
        vsAmbShader = createShader(m_materials[i], ExShader::ST_VSAM, params);
        fpAmbShader = createShader(m_materials[i], ExShader::ST_FPAM, params);
        vsLightShader = createShader(m_materials[i], ExShader::ST_VSLIGHT, params);
        fpLightShader = createShader(m_materials[i], ExShader::ST_FPLIGHT, params);
      }
      
      m_materials[i]->writeOgreScript(params, outMaterial, vsAmbShader ,fpAmbShader, vsLightShader, fpLightShader);
		}
    outMaterial.close();

    if(params.exportProgram != SHADER_NONE)
    {
      std::ofstream outShaderCG;
      std::ofstream outProgram;
  	  
      std::string cgFilePath = makeOutputPath(params.outputDir, params.programOutputDir, optimizeFileName(params.sceneFilename), "cg");
	    outShaderCG.open(cgFilePath.c_str());
	    if (!outShaderCG)
	    {
			  EasyOgreExporterLog("Error opening file: %s\n", cgFilePath.c_str());
		    return false;
	    }
      
      std::string prFilePath = makeOutputPath(params.outputDir, params.programOutputDir, optimizeFileName(params.sceneFilename), "program");
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

        outProgram << m_Shaders[i]->getProgram(optimizeFileName(params.sceneFilename));
        outProgram << "\n";
      }

      outShaderCG.close();
      outProgram.close();
    }

    
		//Copy textures to output dir if required
		if (params.copyTextures)
    {
      EasyOgreExporterLog("Copy material textures files\n");
      std::vector<std::string> lTexDone;

		  for (int i=0; i<m_materials.size(); i++)
		  {
		    for (int j=0; j<m_materials[i]->m_textures.size(); j++)
		    {
          Texture tex = m_materials[i]->m_textures[j];

          //Copy only each files once
          if(std::find(lTexDone.begin(), lTexDone.end(), tex.absFilename) == lTexDone.end())
          {
            bool ddsMode = false;
            
            std::string texName = params.resPrefix;
            texName.append(tex.filename);
            texName = optimizeFileName(texName);

            std::string destFile = makeOutputPath(params.outputDir, params.texOutputDir, texName, "");
            std::string texExt = ToLowerCase(texName.substr(texName.find_last_of(".") + 1));

            //DDS conversion
#ifdef UNICODE
			std::wstring absFilename_w;
			absFilename_w.assign(tex.absFilename.begin(),tex.absFilename.end());
			if(params.convertToDDS && (texExt != "dds") && DoesFileExist(absFilename_w.data()))
#else
            if(params.convertToDDS && (texExt != "dds") && DoesFileExist(tex.absFilename.c_str()))
#endif
            {
              destFile = makeOutputPath(params.outputDir, params.texOutputDir, texName.substr(0, texName.find_last_of(".")), "DDS");

              //load bitmap
              BMMRES status;
 #ifdef UNICODE
			std::string absFilename_s = tex.absFilename.c_str();
			std::wstring absFilename_w;
			absFilename_w.assign(absFilename_s.begin(),absFilename_s.end());
			BitmapInfo bi(absFilename_w.c_str());
#else
			BitmapInfo bi(tex.absFilename.c_str());
#endif
              Bitmap* bitmap = TheManager->Create(&bi); 
              bitmap = TheManager->Load(&bi, &status);
              if (status == BMMRES_SUCCESS) 
              {
                unsigned int width = bitmap->Width();
                unsigned int height = bitmap->Height();

                if(bitmap->Flags() & MAP_FLIPPED)
                  EasyOgreExporterLog("Info texture file %s is flipped\n", tex.filename.c_str());
                if(bitmap->Flags() & MAP_INVERTED)
                  EasyOgreExporterLog("Info texture file %s is inverted\n", tex.filename.c_str());

                int bpp = 4;
                int bpl = width * bpp;

                int atype;
                int btype;
                unsigned char* bBuff = (unsigned char*)bitmap->GetStoragePtr(&btype);
                unsigned char* aBuff = (unsigned char*)bitmap->GetAlphaPtr(&atype); 
                unsigned char* pBuff = (unsigned char*)malloc(width * height * bpp);

                if ((btype != BMM_NO_TYPE) && (bBuff != 0))
                {
                  if(bitmap->Storage()->Type() == BMM_GRAY_8)
                  {
                    //add alpha to buffer and convert to BGRA from grey
                    for (int x = 0; x < width; x++)
                    {
                      for(int y = 0; y < height; y++) 
                      {
                        unsigned long destByte = (x * bpp) + (bpl * y);
                        unsigned long srcByte = x + (width * y);
                        unsigned long srcAlphaByte = x + (width * y);

                        pBuff[destByte] = bBuff[srcByte];
                        pBuff[destByte+1] = bBuff[srcByte];
                        pBuff[destByte+2] = bBuff[srcByte];
                        pBuff[destByte+3] = (atype != BMM_NO_TYPE && aBuff != 0) ? aBuff[srcAlphaByte] : 0xff;
                      }
                    }
                  }
                  else
                  {
                    //add alpha to buffer and convert to BGRA
                    for (int x = 0; x < width; x++)
                    {
                      for(int y = 0; y < height; y++) 
                      {
                        unsigned long destByte = (x * bpp) + (bpl * y);
                        unsigned long srcByte = (x * 3) + (width * 3 * y);
                        unsigned long srcAlphaByte = x + (width * y);

                        pBuff[destByte] = bBuff[srcByte+2];
                        pBuff[destByte+1] = bBuff[srcByte+1];
                        pBuff[destByte+2] = bBuff[srcByte];
                        pBuff[destByte+3] = (atype != BMM_NO_TYPE && aBuff != 0) ? aBuff[srcAlphaByte] : 0xff;
                      }
                    }
                  }

                  nvtt::InputOptions inputOptions;
                  inputOptions.setTextureLayout(nvtt::TextureType_2D, width, height);
                  inputOptions.setMipmapData(pBuff, width, height);
                  inputOptions.setFormat(nvtt::InputFormat_BGRA_8UB);
                  //inputOptions.setAlphaMode(bitmap->HasAlpha() && bitmap->PreMultipliedAlpha() ? nvtt::AlphaMode_Premultiplied : bitmap->HasAlpha() ? nvtt::AlphaMode_Transparency : nvtt::AlphaMode_None);
                  inputOptions.setAlphaMode(bitmap->HasAlpha() ? nvtt::AlphaMode_Premultiplied : nvtt::AlphaMode_None);
                  inputOptions.setMaxExtents(params.maxTextureSize);
                  inputOptions.setRoundMode(nvtt::RoundMode_ToNearestPowerOfTwo);
                  inputOptions.setMipmapGeneration(true);
                  inputOptions.setWrapMode(nvtt::WrapMode_Clamp);
                  
                  nvtt::OutputOptions outputOptions;
                  outputOptions.setFileName(destFile.c_str());

                  nvtt::CompressionOptions compressionOptions;
                  compressionOptions.setQuality(nvtt::Quality_Production);
                  compressionOptions.setFormat(bitmap->HasAlpha() ? nvtt::Format_DXT5 : nvtt::Format_DXT1);
#if(NVTT_VERSION<200)
                  compressionOptions.setTargetDecoder(nvtt::Decoder_D3D9);
#endif                  

#if(NVTT_VERSION<200)
                  nvtt::Context context;
                  ddsMode = context.process(inputOptions, compressionOptions, outputOptions);
#else
                  nvtt::Compressor compressor;
                  ddsMode = compressor.process(inputOptions, compressionOptions, outputOptions);
#endif                  
                  free(pBuff);
                }
                TheManager->DelBitmap(bitmap);
              }
            }

			if(!ddsMode)
			{
				// Copy file texture to output dir
#ifdef UNICODE
				std::wstring absFilename_w;
				absFilename_w.assign(tex.absFilename.begin(),tex.absFilename.end());
				std::wstring destFile_w;
				destFile_w.assign(destFile.begin(),destFile.end());
				if(!CopyFile(absFilename_w.data(), destFile_w.data(), false))
#else
				if(!CopyFile(tex.absFilename.c_str(), destFile.c_str(), false))
#endif
					EasyOgreExporterLog("Error while copying texture file %s\n", tex.absFilename.c_str());
			}

            lTexDone.push_back(tex.absFilename);
          }
		    }
		  }

      lTexDone.clear();
    }

    //textures copy
		return true;
	};

};	//end namespace
