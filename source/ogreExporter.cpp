////////////////////////////////////////////////////////////////////////////////
// ogreExporter.cpp
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

#include "OgreExporter.h"
#include "ExData.h"
#include "EasyOgreExporterLog.h"

#if defined(WIN32)
// For SHGetFolderPath.  Requires Windows XP or greater.
#include <stdarg.h>
#include <Shlobj.h>
#include <direct.h>
#endif // defined(WIN32)

namespace EasyOgreExporter
{

OgreSceneExporter::OgreSceneExporter()
{
}

OgreSceneExporter::~OgreSceneExporter()
{
}

int OgreSceneExporter::ExtCount(void)
{
  return 1;
}

const TCHAR* OgreSceneExporter::Ext(int n)
{		
  return _T("scene");
}

const TCHAR* OgreSceneExporter::LongDesc(void)
{
  return _T("Easy Ogre Exporter");
}

const TCHAR* OgreSceneExporter::ShortDesc(void) 
{			
  return _T("Ogre Scene");
}

const TCHAR* OgreSceneExporter::AuthorName(void)
{			
  return _T("Bastien Bourineau for OpenSpace3D, based on Francesco Giordana and OC3 Entertainment, Inc. work");
}

const TCHAR* OgreSceneExporter::CopyrightMessage(void) 
{	
  return _T("Copyright (c) 2011 OpenSpace3D");
}

const TCHAR* OgreSceneExporter::OtherMessage1(void) 
{		
  return _T("");
}

const TCHAR* OgreSceneExporter::OtherMessage2(void) 
{		
  return _T("");
}

unsigned int OgreSceneExporter::Version(void)
{				
  // Return Version number * 100.  100 = 1.00.
  return 100;
}

void OgreSceneExporter::ShowAbout(HWND hWnd)
{			
  //@todo
}

BOOL OgreSceneExporter::SupportsOptions(int ext, DWORD options)
{  
  return TRUE;
}

int	OgreSceneExporter::DoExport(const TCHAR* name, ExpInterface* pExpInterface, Interface* pInterface, BOOL suppressPrompts, DWORD options)
{
  ParamList params;
  params = ExData::maxInterface.m_params;
  
  params.exportAll = (options & SCENE_EXPORT_SELECTED) ? false : true;

  // Using only a scene filename, construct the other paths
  std::string scenePath = name;
  for (int i=0; i<scenePath.length(); ++i)
  {
    scenePath[i]=tolower(scenePath[i]);
  }

  size_t sceneIndex = scenePath.rfind(".scene", scenePath.length() -1);
  size_t folderIndexForward = scenePath.rfind("/", scenePath.length() -1);
  size_t folderIndexBackward = scenePath.rfind("\\", scenePath.length() -1);
  size_t folderIndex;
  if(folderIndexForward == std::string::npos)
  {
    folderIndex = folderIndexBackward;
  }
  else if(folderIndexBackward == std::string::npos)
  {
    folderIndex = folderIndexForward;
  }
  else
  {
    folderIndex = folderIndexBackward > folderIndexForward ? folderIndexBackward : folderIndexForward;
  }

  if(sceneIndex == std::string::npos || folderIndex == std::string::npos)
  {
    MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Invalid scene filename."), _T("Error"), MB_OK);
    return false;
  }
  
  std::string outDir = scenePath.substr(0, folderIndex);
  std::string texOutDir = "bitmap";
  std::string meshOutDir = "mesh";
  std::string matOutDir = "material";
  std::string partOutDir = "particle";
  std::string sceneFile = scenePath.substr(folderIndex + 1, (sceneIndex - (folderIndex + 1)));
  std::string matPrefix = sceneFile;
  
  // Setup the paramlist.
  params.outputDir = outDir.c_str();
  params.texOutputDir = texOutDir.c_str();
  params.meshOutputDir = meshOutDir.c_str();
  params.materialOutputDir = matOutDir.c_str();
  params.particlesOutputDir = partOutDir.c_str();
  params.matPrefix = matPrefix.c_str();
  params.sceneFilename = sceneFile.c_str();
  
  bool success = false;

  std::string scriptDir = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_SCRIPTS_DIR);
  std::string scriptPath = scriptDir + "\\EasyOgreGUI.ms";

  DispInfo unitsInfo;
  GetUnitDisplayInfo(&unitsInfo);
  switch(unitsInfo.dispType)
  {
    case UNITDISP_METRIC:
      switch(unitsInfo.metricDisp)
      {
        case UNIT_METRIC_DISP_MM:
          params.lum = CM2MM;
        break;

        case UNIT_METRIC_DISP_CM:
          params.lum = CM2CM;
        break;

        case UNIT_METRIC_DISP_M:
          params.lum = CM2M;
        break;

        case UNIT_METRIC_DISP_KM:
          params.lum = CM2KM;
        break;
      }
    break;
  }

  ExData::maxInterface.m_params = params;
  
  /*FileStream scriptFile;
  if(scriptFile.open(scriptPath.c_str(), "rt"))
  {
    ExecuteScript(&scriptFile, &success);
    scriptFile.close();
  }
  else*/
  {
    //MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Script file not found."), _T("Error"), MB_OK);
    success = ExData::maxInterface.exportScene();
  }
  
  return success;
}

// Dummy function for progress bar.
DWORD WINAPI fn(LPVOID arg)
{
  return 0;
}

OgreExporter::OgreExporter() :
  pIGame(NULL),
  sceneData(0),
  ogreConverter(0)
{
}

OgreExporter::~OgreExporter()
{
}

bool OgreExporter::exportScene()
{
  //Init log files
  std::string logFileName = m_params.outputDir;
  logFileName.append("\\easyOgreExporter.log");
  EasyOgreExporter::EasyOgreExporterLogFile::SetPath(logFileName.data());

#if defined(WIN32)
  if(m_params.exportMaterial)
    _mkdir((makeOutputPath(m_params.outputDir, m_params.materialOutputDir, "", "")).c_str());
  
  if(m_params.copyTextures)
    _mkdir((makeOutputPath(m_params.outputDir, m_params.texOutputDir, "", "")).c_str());  
  
  _mkdir((makeOutputPath(m_params.outputDir, m_params.meshOutputDir, "", "")).c_str());
#endif

  // Create Ogre Root
  // Ogre::Root ogreRoot;
  // Create singletons
  Ogre::LogManager logMgr;
  Ogre::LogManager::getSingleton().createLog("Ogre.log", true);
  Ogre::ResourceGroupManager rgm;
  Ogre::MeshManager meshMgr;
  Ogre::SkeletonManager skelMgr;
  Ogre::MaterialManager matMgr;
  Ogre::DefaultHardwareBufferManager hardwareBufMgr;
  // Doug Perkowski  - 03/09/10
  // Creating LodStrategyManager
  // http://www.ogre3d.org/forums/viewtopic.php?f=8&t=55844
  Ogre::LodStrategyManager lodstrategymanager;  

  m_params.currentRootJoints.clear();

  pIGame = GetIGameInterface();

  // Passing in true causing crash on IGameNode->GetNodeParent.  
  // Test for selection in Translate node.
  pIGame->InitialiseIGame(false);
  pIGame->SetStaticFrame(0);

  IGameConversionManager* pConversionManager = GetConversionManager();
  if(m_params.yUpAxis)
  {
    pConversionManager->SetCoordSystem(IGameConversionManager::IGAME_OGL);
  }
  else
  {
    pConversionManager->SetCoordSystem(IGameConversionManager::IGAME_MAX);
  }

  Interval animInterval = GetCOREInterface()->GetAnimRange(); 

  //TODO only in corresponded class
  // Create output files
  m_params.openFiles();

  ogreConverter = new ExOgreConverter(m_params);

  //Init Scene
  if(m_params.exportScene)
    sceneData = new ExScene(ogreConverter, m_params);

  // parse Max scene
  for(int node = 0; node < pIGame->GetTopLevelNodeCount(); ++node)
  {
    IGameNode* pGameNode = pIGame->GetTopLevelNode(node);
    if(pGameNode)
    {
      exportNode(pGameNode, 0);
    }
  }

  if (sceneData)
    sceneData->writeSceneFile();

  if (ogreConverter)
    ogreConverter->writeMaterialFile();

  if (sceneData)
  {
    delete sceneData;
    sceneData = 0;
  }

  if (ogreConverter)
  {
    delete ogreConverter;
    ogreConverter = 0;
  }

  m_params.closeFiles();
  pIGame->ReleaseIGame();
  
  EasyOgreExporterLog("Info: export done.\n");
  MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Export done successfully."), _T("Info"), MB_OK);
  return true;
}

bool OgreExporter::exportNode(IGameNode* pGameNode, TiXmlElement* parent)
{
  if(pGameNode)
  {
    IGameObject* pGameObject = pGameNode->GetIGameObject();
    if(pGameObject)
    {
      bool bShouldExport = true;
      INode* node = pGameNode->GetMaxNode();
      if(node)
      {
        if(node->IsObjectHidden())
        {
          bShouldExport = false;
        }
        // Only export selection if exportAll = false
        if(node->Selected() == 0 && !m_params.exportAll)
        {
          bShouldExport = false;
        }

        if (pGameObject->GetMaxType() == IGameObject::IGAME_MAX_UNKNOWN)
        {
          EasyOgreExporterLog("Unsupported object type. Failed to export.\n");
          bShouldExport = false;
        }
      }
      
      if(bShouldExport)
      {
        EasyOgreExporterLog("Found node: %s\n", pGameNode->GetName());
        switch(pGameObject->GetIGameType())
        {
          case IGameObject::IGAME_MESH:
          {
            IGameMesh* pGameMesh = static_cast<IGameMesh*>(pGameObject);
            if(pGameMesh)
            {
              if(pGameMesh->InitializeData())
              {
                EasyOgreExporterLog("Found mesh node: %s\n", pGameNode->GetName());

                if(ogreConverter)
                  if (!ogreConverter->writeEntityData(pGameNode, pGameObject, pGameMesh))
                    EasyOgreExporterLog("Error, mesh skipped\n");

                if (sceneData)
                {
                  parent = sceneData->writeNodeData(parent, pGameNode, IGameObject::IGAME_MESH);
                  sceneData->writeEntityData(parent, pGameMesh);
                }
              }
            }
          }
          break;
        case IGameObject::IGAME_LIGHT:
          {
            IGameLight* pGameLight = static_cast<IGameLight*>(pGameObject);
            if(pGameLight)
            {
              EasyOgreExporterLog("Found light: %s\n", pGameNode->GetName());
              if (sceneData)
              {
                parent = sceneData->writeNodeData(parent, pGameNode, IGameObject::IGAME_LIGHT);
                sceneData->writeLightData(parent, pGameLight);
              }
            }
          }
          break;
        case IGameObject::IGAME_CAMERA:
          {
            IGameCamera* pGameCamera = static_cast<IGameCamera*>(pGameObject);
            if(pGameCamera)
            {
              EasyOgreExporterLog("Found camera: %s\n", pGameNode->GetName());
              if (sceneData)
              {
                parent = sceneData->writeNodeData(parent, pGameNode, IGameObject::IGAME_CAMERA);
                sceneData->writeCameraData(parent, pGameCamera);
              }
            }
          }
          break;
        case IGameObject::IGAME_HELPER:
          {
            parent = sceneData->writeNodeData(parent, pGameNode, IGameObject::IGAME_HELPER);
          }
          break;
        default:
          break;
        }
      }
      pGameNode->ReleaseIGameObject();

      for(int i = 0; i < pGameNode->GetChildCount(); ++i)
      {
        IGameNode* pChildGameNode = pGameNode->GetNodeChild(i);
        if(pChildGameNode)
        {
          exportNode(pChildGameNode, parent);
        }
      }
    }
  }
  return true;
}

} // end namespace



class EasyOgreExporterClassDesc:public ClassDesc2 
{
  public:
  int 			IsPublic() { return TRUE; }
  void *			Create(BOOL loading = FALSE) { return new EasyOgreExporter::OgreSceneExporter(); }
  const TCHAR *	ClassName() { return _T("OgreExporter"); }
  SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
  Class_ID		ClassID() { return Class_ID(0x6a042a9d, 0x75b54fc4); }
  const TCHAR* 	Category() { return _T("OGRE"); }
  const TCHAR*	InternalName() { return _T("OgreExporter"); }	
  HINSTANCE		HInstance() { return hInstance; }				
};
static EasyOgreExporterClassDesc EasyOgreExporterDesc;
ClassDesc2* GetEasyOgreExporterDesc() { return &EasyOgreExporterDesc; }

