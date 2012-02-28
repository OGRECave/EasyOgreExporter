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
#include "ExTools.h"
#include "tinyxml.h"

#include "../resources/resource.h"
#include "3dsmaxport.h"

#if defined(WIN32)
// For SHGetFolderPath.  Requires Windows XP or greater.
#include <stdarg.h>
#include <Shlobj.h>
#include <direct.h>
#endif // defined(WIN32)


//Exporter version
float EXVERSION = 0.7f;

namespace EasyOgreExporter
{

  /**
  * Configuration interface
  **/
  INT_PTR CALLBACK IGameExporterOptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    ParamList* exp = DLGetWindowLongPtr<ParamList*>(hWnd); 

    switch(message)
    {
	    case WM_INITDIALOG:
		    exp = (ParamList*)lParam;
        DLSetWindowLongPtr(hWnd, lParam); 
		    CenterWindow(hWnd, GetParent(hWnd));

        //fill Ogre version combo box
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.8");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.7");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.4");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.0");

        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_SETCURSEL, (int)exp->meshVersion, 0);

        //fill material prefix
        SendDlgItemMessage(hWnd, IDC_MATPREFIX, WM_SETTEXT, 0, (LPARAM)(char*)exp->matPrefix.c_str());

        //fill material sub dir
        SendDlgItemMessage(hWnd, IDC_MATDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->materialOutputDir.c_str());

        //fill texture sub dir
        SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->texOutputDir.c_str());

        //fill mesh subdir
        SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->meshOutputDir.c_str());
        
        //advanced config
		    CheckDlgButton(hWnd, IDC_SHAREDGEOM, exp->useSharedGeom);
        CheckDlgButton(hWnd, IDC_GENLOD, exp->generateLOD);
        CheckDlgButton(hWnd, IDC_EDGELIST, exp->buildEdges);
        CheckDlgButton(hWnd, IDC_TANGENT, exp->buildTangents);
        CheckDlgButton(hWnd, IDC_SPLITMIRROR, exp->tangentsSplitMirrored);
        CheckDlgButton(hWnd, IDC_SPLITROT, exp->tangentsSplitRotated);
        CheckDlgButton(hWnd, IDC_STOREPARITY, exp->tangentsUseParity);
        CheckDlgButton(hWnd, IDC_RESAMPLE_ANIMS, exp->resampleAnims);
    		
		    //Versioning
		    TCHAR Title [256];
        _stprintf(Title, "Easy Ogre Exporter version %.1f", EXVERSION);
		    SetWindowText(hWnd, Title);
		    return TRUE;

	    case WM_COMMAND:
		    switch (LOWORD(wParam))
        {
			    case IDOK:
            {
              int ogreVerIdx = SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_GETCURSEL, 0, 0);
              if (ogreVerIdx != CB_ERR)
              {
                switch (ogreVerIdx)
                {
                  case 0:
                    exp->meshVersion = TOGRE_1_8;
                    break;
                  case 1:
                    exp->meshVersion = TOGRE_1_7;
                    break;
                  case 2:
                    exp->meshVersion = TOGRE_1_4;
                    break;
                  case 3:
                    exp->meshVersion = TOGRE_1_0;
                    break;

                  default:
                    exp->meshVersion = TOGRE_1_8;
                }
              }

					    TSTR temp;
              int len = 0;
              
              len = SendDlgItemMessage(hWnd, IDC_MATPREFIX, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_MATPREFIX, WM_GETTEXT, len+1, (LPARAM)temp.data());
              exp->matPrefix = temp;

              len = SendDlgItemMessage(hWnd, IDC_MATDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_MATDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
              exp->materialOutputDir = temp;

              len = SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
              exp->texOutputDir = temp;

              len = SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
              exp->meshOutputDir = temp;

              exp->useSharedGeom = IsDlgButtonChecked(hWnd, IDC_SHAREDGEOM) ? true : false;
              exp->generateLOD = IsDlgButtonChecked(hWnd, IDC_GENLOD) ? true : false;
              exp->buildEdges = IsDlgButtonChecked(hWnd, IDC_EDGELIST) ? true : false;
              exp->buildTangents = IsDlgButtonChecked(hWnd, IDC_TANGENT) ? true : false;
              exp->tangentsSplitMirrored = IsDlgButtonChecked(hWnd, IDC_SPLITMIRROR) ? true : false;
              exp->tangentsSplitRotated = IsDlgButtonChecked(hWnd, IDC_SPLITROT) ? true : false;
              exp->tangentsUseParity = IsDlgButtonChecked(hWnd, IDC_STOREPARITY) ? true : false;
              exp->resampleAnims = IsDlgButtonChecked(hWnd, IDC_RESAMPLE_ANIMS) ? true : false;

              EndDialog(hWnd, 1);
            }
				    break;
			    case IDCANCEL:
				    EndDialog(hWnd,0);
				    break;
		    }
    	
	    default:
		    return FALSE;

    }
    return TRUE;
  }

// Dummy function for progress bar
DWORD WINAPI progressCb(LPVOID arg)
{
	return(0);
}

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
  params.matPrefix = matPrefix.c_str();
  params.sceneFilename = sceneFile.c_str();
  
  int unitType = 0;
  float unitScale = 0;
  GetMasterUnitInfo(&unitType, &unitScale);
  DispInfo unitsInfo;
  GetUnitDisplayInfo(&unitsInfo);
  params.lum = ConvertToMeter(unitsInfo.metricDisp, unitType) * unitScale;

  ExData::maxInterface.m_params = params;

	if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), pInterface->GetMAXHWnd(), IGameExporterOptionsDlgProc, (LPARAM)&(ExData::maxInterface.m_params)))
  {
		return 1;
	}

  return ExData::maxInterface.exportScene();
}

// Dummy function for progress bar.
DWORD WINAPI fn(LPVOID arg)
{
  return 0;
}

OgreExporter::OgreExporter() :
  pIGame(NULL),
  sceneData(0),
  ogreConverter(0),
  nodeCount(0)
{
}

OgreExporter::~OgreExporter()
{
}

void OgreExporter::initIGameConf(std::string path)
{
  TiXmlDocument xmlDoc;
  TiXmlElement* igameProperties = new TiXmlElement("IGameProperties");
  xmlDoc.LinkEndChild(igameProperties);
  TiXmlElement* igameUserData = new TiXmlElement("ExportUserData");
  igameProperties->LinkEndChild(igameUserData);

  // renderingDistance
  TiXmlElement* renderDist = new TiXmlElement("UserProperty");
  TiXmlElement* renderDistId = new TiXmlElement("id");
  TiXmlText* renderDistIdText = new TiXmlText("102");
  renderDistId->LinkEndChild(renderDistIdText);
  renderDist->LinkEndChild(renderDistId);
  
  TiXmlElement* renderDistSName = new TiXmlElement("simplename");
  TiXmlText* renderDistSNameText = new TiXmlText("renderingDistance");
  renderDistSName->LinkEndChild(renderDistSNameText);
  renderDist->LinkEndChild(renderDistSName);

  TiXmlElement* renderDistName = new TiXmlElement("keyName");
  TiXmlText* renderDistNameText = new TiXmlText("renderingDistance");
  renderDistName->LinkEndChild(renderDistNameText);
  renderDist->LinkEndChild(renderDistName);

  TiXmlElement* renderDistType = new TiXmlElement("type");
  TiXmlText* renderDistTypeText = new TiXmlText("float");
  renderDistType->LinkEndChild(renderDistTypeText);
  renderDist->LinkEndChild(renderDistType);

  igameUserData->LinkEndChild(renderDist);

  // noLOD
  TiXmlElement* noLod = new TiXmlElement("UserProperty");
  TiXmlElement* noLodId = new TiXmlElement("id");
  TiXmlText* noLodIdText = new TiXmlText("103");
  noLodId->LinkEndChild(noLodIdText);
  noLod->LinkEndChild(noLodId);
  
  TiXmlElement* noLodSName = new TiXmlElement("simplename");
  TiXmlText* noLodSNameText = new TiXmlText("noLOD");
  noLodSName->LinkEndChild(noLodSNameText);
  noLod->LinkEndChild(noLodSName);

  TiXmlElement* noLodName = new TiXmlElement("keyName");
  TiXmlText* noLodNameText = new TiXmlText("noLOD");
  noLodName->LinkEndChild(noLodNameText);
  noLod->LinkEndChild(noLodName);

  TiXmlElement* noLodType = new TiXmlElement("type");
  TiXmlText* noLodTypeText = new TiXmlText("bool");
  noLodType->LinkEndChild(noLodTypeText);
  noLod->LinkEndChild(noLodType);

  igameUserData->LinkEndChild(noLod);

  xmlDoc.SaveFile(path.c_str());
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
  Ogre::LodStrategyManager lodstrategymanager;  

  m_params.currentRootJoints.clear();

  std::string plugConfDir = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
  std::string xmlConfPath = plugConfDir + "\\EasyOgreExporter\\IGameProp.xml";
  _mkdir((std::string(plugConfDir + "\\EasyOgreExporter")).c_str());

  initIGameConf(xmlConfPath);

  pIGame = GetIGameInterface();
  pIGame->SetPropertyFile(xmlConfPath.c_str());

  // Passing in true causing crash on IGameNode->GetNodeParent.  
  // Test for selection in Translate node.
  pIGame->InitialiseIGame(false);
  pIGame->SetStaticFrame(0);

  std::string sTitle = "Easy Ogre Exporter: " + m_params.sceneFilename;
  GetCOREInterface()->ProgressStart((char*)sTitle.c_str(), TRUE, progressCb, 0);

  //WARNING this apply transform only matrix from on iGameNode not on max iNode
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
  {
    GetCOREInterface()->ProgressUpdate(95, FALSE, "Writing scene file.");
    sceneData->writeSceneFile();
  }

  if (ogreConverter)
  {
    GetCOREInterface()->ProgressUpdate(98, FALSE, "Writing material file.");
    ogreConverter->writeMaterialFile();
  }

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

  pIGame->ReleaseIGame();
  
  GetCOREInterface()->ProgressUpdate(99, FALSE, "Done.");
  EasyOgreExporterLog("Info: export done.\n");

  //close the progress bar
  GetCOREInterface()->ProgressEnd();

  MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Export done successfully."), _T("Info"), MB_OK);
  return true;
}

bool OgreExporter::exportNode(IGameNode* pGameNode, TiXmlElement* parent)
{
  GetCOREInterface()->ProgressUpdate((int)(((float)nodeCount / pIGame->GetTotalNodeCount()) * 90.0f), FALSE, 0); 

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
        GetCOREInterface()->ProgressUpdate((int)(((float)nodeCount / pIGame->GetTotalNodeCount()) * 90.0f), FALSE, pGameNode->GetName()); 

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
            if(m_params.exportLights)
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
          }
          break;
        case IGameObject::IGAME_CAMERA:
          {
            if(m_params.exportCameras)
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
    nodeCount++;
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

