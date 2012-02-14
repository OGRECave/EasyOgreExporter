////////////////////////////////////////////////////////////////////////////////
// paramlist.cpp
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

#include "paramlist.h"
#include "EasyOgreExporterLog.h"
/***** Class ParamList *****/
// method to parse arguments from command line
namespace EasyOgreExporter
{
	// Helper function for getting the filename from a full path.
	std::string StripToTopParent(const std::string& filepath)
	{
		int ri = filepath.find_last_of('\\');
		int ri2 = filepath.find_last_of('/');
		if(ri2 > ri)
		{
			ri = ri2;
		}
		return filepath.substr(ri+1);
	}

	// Helper function to replace special chars for file names
	std::string optimizeFileName(const std::string& filename)
	{
    std::string newFilename = filename;
    std::replace(newFilename.begin(), newFilename.end(), ' ', '_');
    std::replace(newFilename.begin(), newFilename.end(), '\'', '_');
    std::replace(newFilename.begin(), newFilename.end(), '"', '_');
		return newFilename;;
	}

	// Helper function to generate full filepath
  std::string makeOutputPath(std::string common, std::string dir, std::string file, std::string ext)
  {
    std::string filePath = "";
    filePath.append(common);

    if (!common.empty())
    if ((common.compare((common.length() -2), 1, "/") != 0) && (common.compare((common.length() -2), 1, "\\") != 0))
      filePath.append("\\");

    filePath.append(dir);

    if (!dir.empty())
    if ((dir.compare((dir.length() -2), 1, "/") != 0) && (dir.compare((dir.length() -2), 1, "\\") != 0))
      filePath.append("\\");

    filePath.append(file);

    if(!ext.empty())
    {
      filePath.append(".");
      filePath.append(ext);
    }

    return filePath;
  }

	// method to open output files for writing
	bool ParamList::openFiles()
	{
		if (exportParticles)
		{
      std::string filePath = makeOutputPath(outputDir, particlesOutputDir, sceneFilename, "particle");
			outParticles.open(filePath.c_str());
			if (!outParticles)
			{
				EasyOgreExporterLog("Error opening file: %s\n", filePath.c_str());
				return false;
			}
		}
		return true;
	}

	// method to close open output files
	bool ParamList::closeFiles()
	{	
		if (exportParticles)
			outParticles.close();
		
		return true;
	}

}	//end namespace
