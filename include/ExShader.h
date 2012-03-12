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
			  ST_FPLIGHT
		  };
    protected:
      bool bRef;
      bool bNormal;
      bool bDiffuse;
      bool bSpecular;
      ShaderType m_type;

      std::string m_name;
      std::string m_content;
      std::string m_program;
      std::string m_params;
    private:
    
    public:
      ExShader(std::string name);
		  ~ExShader();

      virtual void constructShader(ExMaterial* mat);
      std::string& getName();
      std::string& getContent();
      virtual std::string& getUniformParams();
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExVsShader: public ExShader
	{
	  public:
    protected:
    private:
    
    public:
      ExVsShader(std::string name);
		  ~ExVsShader();

      virtual void constructShader(ExMaterial* mat);
      virtual std::string& getUniformParams();
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};

  class ExFpShader: public ExShader
	{
	  public:
    protected:
    private:

    public:
		  //constructor
      ExFpShader(std::string name);

		  //destructor
		  ~ExFpShader();

      virtual void constructShader(ExMaterial* mat);
      virtual std::string& getUniformParams();
      virtual std::string& getProgram(std::string baseName);
    protected:
    private:
	};
};

#endif