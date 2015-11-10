////////////////////////////////////////////////////////////////////////////////
// ExShader.h
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

#ifndef _EXSHADER_H
#define _EXSHADER_H

#include "ExPrerequisites.h"
#include "paramList.h"

namespace EasyOgreExporter
{
  class ExMaterial;

  class ExShader
	{
	  public:
      enum ShaderType
		  {
			  ST_NONE,
			  // ambient vertex shader
			  ST_VSAM,
			  // ambient pixel shader
			  ST_FPAM,
			  // lighting vertex shader
			  ST_VSLIGHT,
			  // lighting pixel shader
			  ST_FPLIGHT,
        // lighting vertex shader multipass
        ST_VSLIGHT_MULTI,
        // lighting pixel shader multipass
        ST_FPLIGHT_MULTI
		  };

      enum ShaderPass
		  {
			  SP_NONE,
			  // ambient pass
			  SP_AMBIENT,
			  // lighting pass
			  SP_LIGHT,
			  // lighting multi pass
        SP_LIGHT_MULTI,
        // decal pass
			  SP_DECAL,
        // non supported
        SP_NOSUPPORT
		  };
    protected:
      bool bFresnel;
      bool bRef;
      bool bNormal;
      bool bAmbient;
      bool bDiffuse;
      bool bSpecular;
      bool bNeedHighProfile;
      ShaderType m_type;

      std::string m_name;
      std::string m_content;
      std::string m_contentGles;
      std::string m_program;
      std::string m_params;
    private:
    
    public:
      ExShader(std::string name);
		  ~ExShader();

      virtual void constructShader(ExMaterial* mat);
      virtual void constructShaderGles(ExMaterial* mat);
      std::string& getName();
      ShaderType getType();
      std::string& getContent();
      std::string& getContentGles();
      virtual std::string& getUniformParams(ExMaterial* mat);
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExVsAmbShader: public ExShader
	{
	  public:
    protected:
    private:
    
    public:
      ExVsAmbShader(std::string name);
		  ~ExVsAmbShader();

      virtual void constructShader(ExMaterial* mat);
      virtual std::string& getUniformParams(ExMaterial* mat);
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExFpAmbShader: public ExShader
	{
	  public:
    protected:
    private:

    public:
      ExFpAmbShader(std::string name);
		  ~ExFpAmbShader();

      virtual void constructShader(ExMaterial* mat);
      virtual std::string& getUniformParams(ExMaterial* mat);
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExVsLightShader: public ExShader
	{
	  public:
    protected:
    private:
    
    public:
      ExVsLightShader(std::string name);
		  virtual ~ExVsLightShader();

      virtual void constructShader(ExMaterial* mat);
      virtual void constructShaderGles(ExMaterial* mat);
      virtual std::string& getUniformParams(ExMaterial* mat);
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExFpLightShader: public ExShader
	{
	  public:
    protected:
    private:

    public:
      ExFpLightShader(std::string name);
		  ~ExFpLightShader();

      virtual void constructShader(ExMaterial* mat);
      virtual void constructShaderGles(ExMaterial* mat);
      virtual std::string& getUniformParams(ExMaterial* mat);
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExVsLightShaderMulti : public ExShader
  {
  public:
  protected:
  private:

  public:
    ExVsLightShaderMulti(std::string name);
    virtual ~ExVsLightShaderMulti();

    virtual void constructShader(ExMaterial* mat);
    virtual std::string& getUniformParams(ExMaterial* mat);
    virtual std::string& getProgram(std::string baseName);
  protected:
  private:
  };

  class ExFpLightShaderMulti : public ExShader
  {
  public:
  protected:
  private:

  public:
    ExFpLightShaderMulti(std::string name);
    ~ExFpLightShaderMulti();

    virtual void constructShader(ExMaterial* mat);
    virtual std::string& getUniformParams(ExMaterial* mat);
    virtual std::string& getProgram(std::string baseName);
  protected:
  private:
  };
};

#endif