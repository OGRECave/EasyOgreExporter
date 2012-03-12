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

//TODO virtual class
namespace EasyOgreExporter
{
  class ExMaterial;

  class ExVsShader
	{
	  public:
    protected:
    private:
      bool bRef;
      bool bNormal;
      bool bDiffuse;
      bool bSpecular;
    
    public:
      ExVsShader(std::string name, ExMaterial* mat);

		  ~ExVsShader();

      std::string& getName();
      std::string& getContent();
      std::string& getUniformParams();
      std::string& getProgram(std::string baseName);
    protected:
    private:
      std::string m_name;
      std::string m_content;
      std::string m_program;
      std::string m_params;
	};

  class ExFpShader
	{
	  public:
    protected:
    private:
      bool bRef;
      bool bNormal;
      bool bDiffuse;
      bool bSpecular;

    public:
		  //constructor
      ExFpShader(std::string name, ExMaterial* mat);

		  //destructor
		  ~ExFpShader();

      std::string& getName();
      std::string& getContent();
      std::string& getUniformParams();
      std::string& getProgram(std::string baseName);
    protected:
    private:
      std::string m_name;
      std::string m_content;
      std::string m_program;
      std::string m_params;
	};

};

#endif