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

#include "OgreLodStrategyManager.h"
#include "OgrePixelCountLodStrategy.h"

#if defined(WIN32)
// For SHGetFolderPath.  Requires Windows XP or greater.
#include <stdarg.h>
#include <Shlobj.h>
#include <direct.h>
#endif // defined(WIN32)


//Exporter version
float EXVERSION = 3.23f;

namespace EasyOgreExporter
{

  /**
  * Configuration interface
  **/
  INT_PTR CALLBACK IGameExporterOptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    ParamList* exp = DLGetWindowLongPtr<ParamList*>(hWnd); 
    ISpinnerControl* spin;

    switch(message)
    {
	    case WM_INITDIALOG:
      {
		    exp = (ParamList*)lParam;
        DLSetWindowLongPtr(hWnd, lParam); 
		    CenterWindow(hWnd, GetParent(hWnd));

        //fill Ogre version combo box
#ifdef UNICODE
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)L"Ogre Latest");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)L"Ogre 1.10");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)L"Ogre 1.8");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)L"Ogre 1.7");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)L"Ogre 1.4");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)L"Ogre 1.0");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_SETCURSEL, (int)exp->meshVersion, 0);

        //fill material prefix
		    std::wstring resPrefix_w;
		    resPrefix_w.assign(exp->resPrefix.begin(),exp->resPrefix.end());
		    SendDlgItemMessage(hWnd, IDC_RESPREFIX, WM_SETTEXT, 0, (LPARAM)resPrefix_w.data());

        //fill material sub dir
		    std::wstring materialOutputDir_w;
		    materialOutputDir_w.assign(exp->materialOutputDir.begin(),exp->materialOutputDir.end());
        SendDlgItemMessage(hWnd, IDC_MATDIR, WM_SETTEXT, 0, (LPARAM)materialOutputDir_w.data());

        //fill texture sub dir
		    std::wstring texOutputDir_w;
		    texOutputDir_w.assign(exp->texOutputDir.begin(),exp->texOutputDir.end());
        SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_SETTEXT, 0, (LPARAM)texOutputDir_w.data());

        //fill mesh subdir
		    std::wstring meshOutputDir_w;
		    meshOutputDir_w.assign(exp->meshOutputDir.begin(),exp->meshOutputDir.end());
        SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_SETTEXT, 0, (LPARAM)meshOutputDir_w.data());

        //fill prog subdir
		    std::wstring programOutputDir_w;
		    programOutputDir_w.assign(exp->programOutputDir.begin(),exp->programOutputDir.end());
		    SendDlgItemMessage(hWnd, IDC_PROGDIR, WM_SETTEXT, 0, (LPARAM)programOutputDir_w.data());
#else
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre Latest");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.10");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.8");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.7");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.4");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_ADDSTRING, 0, (LPARAM)"Ogre 1.0");
        SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_SETCURSEL, (int)exp->meshVersion, 0);

		    //fill material prefix
        SendDlgItemMessage(hWnd, IDC_RESPREFIX, WM_SETTEXT, 0, (LPARAM)(char*)exp->resPrefix.c_str());

        //fill material sub dir
        SendDlgItemMessage(hWnd, IDC_MATDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->materialOutputDir.c_str());

        //fill texture sub dir
        SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->texOutputDir.c_str());

        //fill mesh subdir
        SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->meshOutputDir.c_str());

        //fill prog subdir
        SendDlgItemMessage(hWnd, IDC_PROGDIR, WM_SETTEXT, 0, (LPARAM)(char*)exp->programOutputDir.c_str());
#endif
        spin = GetISpinner(GetDlgItem(hWnd, IDC_RESAMPLE_SPIN));
        spin->LinkToEdit(GetDlgItem(hWnd, IDC_RESAMPLE_STEP), EDITTYPE_INT);
        spin->SetLimits(1, 100, TRUE);
        spin->SetScale(1.0f);
        spin->SetValue(exp->resampleStep, FALSE);
        ReleaseISpinner(spin);

        //advanced config
        CheckDlgButton(hWnd, IDC_YUPAXIS, exp->yUpAxis);
		    CheckDlgButton(hWnd, IDC_SHAREDGEOM, exp->useSharedGeom);
        CheckDlgButton(hWnd, IDC_GENLOD, exp->generateLOD);
        CheckDlgButton(hWnd, IDC_EDGELIST, exp->buildEdges);
        CheckDlgButton(hWnd, IDC_TANGENT, exp->buildTangents);
        CheckDlgButton(hWnd, IDC_SPLITMIRROR, exp->tangentsSplitMirrored);
        CheckDlgButton(hWnd, IDC_SPLITROT, exp->tangentsSplitRotated);
        CheckDlgButton(hWnd, IDC_STOREPARITY, exp->tangentsUseParity);

        CheckDlgButton(hWnd, IDC_CONVDDS, exp->convertToDDS);
        CheckDlgButton(hWnd, IDC_RESAMPLE_ANIMS, exp->resampleAnims);
        CheckDlgButton(hWnd, IDC_LOGS, exp->enableLogs);
    		
#ifdef UNICODE
        //fill Shader mode combo box
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)L"None");
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)L"Only for Normal/Specular");
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)L"All materials (3 lights)");
        //SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)L"All materials (multi pass)");
        
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_SETCURSEL, (int)exp->exportProgram, 0);

        //fill Max texture size combo box
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"64");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"128");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"256");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"512");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"1024");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"2048");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)L"4096");

        //fill Mipmaps combo box
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"Max");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"None");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"2");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"4");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"8");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"16");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)L"32");
#else
        //fill Shader mode combo box
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)"None");
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)"Only for Normal/Specular");
        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_ADDSTRING, 0, (LPARAM)"All materials (3 lights)");

        SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_SETCURSEL, (int)exp->exportProgram, 0);

        //fill Max texture size combo box
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"64");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"128");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"256");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"512");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"1024");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"2048");
        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_ADDSTRING, 0, (LPARAM)"4096");

        //fill Max texture size combo box
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_SETMINVISIBLE, 30, 0);
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_RESETCONTENT, 0, 0);
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"Max");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"None");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"2");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"4");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"8");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"16");
        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_ADDSTRING, 0, (LPARAM)"32");
#endif

        // Enable or disable the DDS options
        EnableWindow(GetDlgItem(hWnd, IDC_TEXSIZE), exp->convertToDDS ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_NUMMIPS), exp->convertToDDS ? TRUE : FALSE);

        int texSel = 0;
        if (exp->maxTextureSize == 128)
          texSel = 1;
        else if (exp->maxTextureSize == 256)
          texSel = 2;
        else if (exp->maxTextureSize == 512)
          texSel = 3;
        else if (exp->maxTextureSize == 1024)
          texSel = 4;
        else if (exp->maxTextureSize == 2048)
          texSel = 5;
        else if (exp->maxTextureSize == 4096)
          texSel = 6;

        SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_SETCURSEL, texSel, 0);

        int mipsSel = 0;
        if (exp->maxMipmaps == -1)
          mipsSel = 0;
        else if (exp->maxMipmaps == 0)
          mipsSel = 1;
        else if (exp->maxMipmaps == 2)
          mipsSel = 2;
        else if (exp->maxMipmaps == 4)
          mipsSel = 3;
        else if (exp->maxMipmaps == 8)
          mipsSel = 4;
        else if (exp->maxMipmaps == 16)
          mipsSel = 5;
        else if (exp->maxMipmaps == 32)
          mipsSel = 6;

        SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_SETCURSEL, mipsSel, 0);

		    //Versioning
		    TCHAR Title [256];
        _stprintf(Title, _T("Easy Ogre Exporter version %.2f"), EXVERSION);
		    SetWindowText(hWnd, Title);
		    return TRUE;
      }
	    case WM_COMMAND:
		    switch (LOWORD(wParam))
        {
          case IDC_CONVDDS:
          {
            // Enable or disable the DDS options
            exp->convertToDDS = IsDlgButtonChecked(hWnd, IDC_CONVDDS) ? true : false;
            EnableWindow(GetDlgItem(hWnd, IDC_TEXSIZE), exp->convertToDDS ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_NUMMIPS), exp->convertToDDS ? TRUE : FALSE);
          }
          break;
			    case IDOK:
            {
              int ogreVerIdx = SendDlgItemMessage(hWnd, IDC_OGREVERSION, CB_GETCURSEL, 0, 0);
              if (ogreVerIdx != CB_ERR)
              {
                switch (ogreVerIdx)
                {
                  case 0:
                    exp->meshVersion = TOGRE_LASTEST;
                    break;
                  case 1:
                    exp->meshVersion = TOGRE_1_10;
                    break;
                  case 2:
                    exp->meshVersion = TOGRE_1_8;
                    break;
                  case 3:
                    exp->meshVersion = TOGRE_1_7;
                    break;
                  case 4:
                    exp->meshVersion = TOGRE_1_4;
                    break;
                  case 5:
                    exp->meshVersion = TOGRE_1_0;
                    break;

                  default:
                    exp->meshVersion = TOGRE_1_10;
                }
              }

              TSTR temp;
              int len = 0;
              
              len = SendDlgItemMessage(hWnd, IDC_RESPREFIX, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_RESPREFIX, WM_GETTEXT, len+1, (LPARAM)temp.data());
#ifdef UNICODE
			        std::wstring temp_w = temp.data();
			        std::string temp_s;
			        temp_s.assign(temp_w.begin(),temp_w.end());
              exp->resPrefix = temp_s;
#else
              exp->resPrefix = temp;
#endif

              len = SendDlgItemMessage(hWnd, IDC_MATDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_MATDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
#ifdef UNICODE
			        temp_w = temp.data();
			        temp_s.assign(temp_w.begin(),temp_w.end());
              exp->materialOutputDir = temp_s;
#else
              exp->materialOutputDir = temp;
#endif

              len = SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_TEXDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
#ifdef UNICODE
			        temp_w = temp.data();
			        temp_s.assign(temp_w.begin(),temp_w.end());
              exp->texOutputDir = temp_s;
#else
              exp->texOutputDir = temp;
#endif

              len = SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_MESHDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
#ifdef UNICODE
			        temp_w = temp.data();
			        temp_s.assign(temp_w.begin(),temp_w.end());
              exp->meshOutputDir = temp_s;
#else
              exp->meshOutputDir = temp;
#endif
              len = SendDlgItemMessage(hWnd, IDC_PROGDIR, WM_GETTEXTLENGTH, 0, 0);
					    temp.Resize(len+1);
					    SendDlgItemMessage(hWnd, IDC_PROGDIR, WM_GETTEXT, len+1, (LPARAM)temp.data());
#ifdef UNICODE
			        temp_w = temp.data();
			        temp_s.assign(temp_w.begin(),temp_w.end());
              exp->programOutputDir = temp_s;
#else
              exp->programOutputDir = temp;
#endif

              spin = GetISpinner(GetDlgItem(hWnd, IDC_RESAMPLE_SPIN));
              exp->resampleStep = spin->GetIVal();
              ReleaseISpinner(spin);

              exp->yUpAxis = IsDlgButtonChecked(hWnd, IDC_YUPAXIS) ? true : false;
              exp->useSharedGeom = IsDlgButtonChecked(hWnd, IDC_SHAREDGEOM) ? true : false;
              exp->generateLOD = IsDlgButtonChecked(hWnd, IDC_GENLOD) ? true : false;
              exp->buildEdges = IsDlgButtonChecked(hWnd, IDC_EDGELIST) ? true : false;
              exp->buildTangents = IsDlgButtonChecked(hWnd, IDC_TANGENT) ? true : false;
              exp->tangentsSplitMirrored = IsDlgButtonChecked(hWnd, IDC_SPLITMIRROR) ? true : false;
              exp->tangentsSplitRotated = IsDlgButtonChecked(hWnd, IDC_SPLITROT) ? true : false;
              exp->tangentsUseParity = IsDlgButtonChecked(hWnd, IDC_STOREPARITY) ? true : false;
              exp->convertToDDS = IsDlgButtonChecked(hWnd, IDC_CONVDDS) ? true : false;
              exp->resampleAnims = IsDlgButtonChecked(hWnd, IDC_RESAMPLE_ANIMS) ? true : false;
              exp->enableLogs = IsDlgButtonChecked(hWnd, IDC_LOGS) ? true : false;

              int shaderIdx = SendDlgItemMessage(hWnd, IDC_SHADERMODE, CB_GETCURSEL, 0, 0);
              if (shaderIdx != CB_ERR)
              {
                switch (shaderIdx)
                {
                  case 0:
                    exp->exportProgram = SHADER_NONE;
                    break;
                  case 1:
                    exp->exportProgram = SHADER_BUMP;
                    break;
                  case 2:
                    exp->exportProgram = SHADER_ALL;
                    break;
                  /*case 3:
                    exp->exportProgram = SHADER_ALL_MULTI;
                    break;
                    */

                  default:
                    exp->exportProgram = SHADER_ALL;
                }
              }

              int texIdx = SendDlgItemMessage(hWnd, IDC_TEXSIZE, CB_GETCURSEL, 0, 0);
              if (texIdx != CB_ERR)
              {
                switch (texIdx)
                {
                  case 0:
                    exp->maxTextureSize = 64;
                    break;
                  case 1:
                    exp->maxTextureSize = 128;
                    break;
                  case 2:
                    exp->maxTextureSize = 256;
                    break;
                  case 3:
                    exp->maxTextureSize = 512;
                    break;
                  case 4:
                    exp->maxTextureSize = 1024;
                    break;
                  case 5:
                    exp->maxTextureSize = 2048;
                    break;
                  case 6:
                    exp->maxTextureSize = 4096;
                    break;

                  default:
                    exp->maxTextureSize = 2048;
                }
              }

              int mipsIdx = SendDlgItemMessage(hWnd, IDC_NUMMIPS, CB_GETCURSEL, 0, 0);
              if (mipsIdx != CB_ERR)
              {
                switch (mipsIdx)
                {
                  case 0:
                    exp->maxMipmaps = -1;
                    break;
                  case 1:
                    exp->maxMipmaps = 0;
                    break;
                  case 2:
                    exp->maxMipmaps = 2;
                    break;
                  case 3:
                    exp->maxMipmaps = 4;
                    break;
                  case 4:
                    exp->maxMipmaps = 8;
                    break;
                  case 5:
                    exp->maxMipmaps = 16;
                    break;
                  case 6:
                    exp->maxMipmaps = 32;
                    break;

                  default:
                    exp->maxMipmaps = -1;
                }
              }

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
  return _T("Copyright (c) 2021 OpenSpace3D");
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
#ifdef UNICODE
  std::wstring scenePath_w = name;
  std::string scenePath;
  scenePath.assign(scenePath_w.begin(),scenePath_w.end());
#else
  std::string scenePath = name;
#endif
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
  std::string progOutDir = "program";
  std::string partOutDir = "particle";
  std::string sceneFile = scenePath.substr(folderIndex + 1, (sceneIndex - (folderIndex + 1)));
  std::string resPrefix = sceneFile;
  
  // Setup the paramlist.
  params.outputDir = outDir.c_str();
  params.texOutputDir = texOutDir.c_str();
  params.meshOutputDir = meshOutDir.c_str();
  params.materialOutputDir = matOutDir.c_str();
  params.programOutputDir = progOutDir.c_str();
  params.resPrefix = resPrefix.c_str();
  params.sceneFilename = sceneFile.c_str();
  
  int unitType = 0;
  float unitScale = 0;
  GetMasterUnitInfo(&unitType, &unitScale);
  params.lum = ConvertToMeter(unitType) * unitScale;

#ifdef UNICODE
  std::wstring plugConfDir_w = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
  std::string plugConfDir;
  plugConfDir.assign(plugConfDir_w.begin(),plugConfDir_w.end());
#else
  std::string plugConfDir = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
#endif
  std::string xmlConfPath = plugConfDir + "\\EasyOgreExporter\\config.xml";
  loadExportConf(xmlConfPath, params);

  ExData::maxInterface.m_params = params;

	if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), pInterface->GetMAXHWnd(), IGameExporterOptionsDlgProc, (LPARAM)&(ExData::maxInterface.m_params)))
  {
		return 1;
	}

  return ExData::maxInterface.exportScene();
}

void OgreSceneExporter::loadExportConf(std::string path, ParamList &param)
{
  TiXmlDocument xmlDoc;
  if(xmlDoc.LoadFile(path.c_str()))
  {
    TiXmlElement* rootElem = xmlDoc.RootElement();
    TiXmlElement* child = rootElem->FirstChildElement("IDC_OGREVERSION");
    if(child)
    {
      if(child->GetText())
      {
        switch (atoi(child->GetText()))
        {
          case 0:
            param.meshVersion = TOGRE_LASTEST;
            break;
          case 1:
            param.meshVersion = TOGRE_1_10;
            break;
          case 2:
            param.meshVersion = TOGRE_1_8;
            break;
          case 3:
            param.meshVersion = TOGRE_1_7;
            break;
          case 4:
            param.meshVersion = TOGRE_1_4;
            break;
          case 5:
            param.meshVersion = TOGRE_1_0;
            break;
        }
      }
    }

    child = rootElem->FirstChildElement("IDC_RESPREFIX");
    if(child)
      param.resPrefix = child->GetText() ? child->GetText() : "";

    child = rootElem->FirstChildElement("IDC_MATDIR");
    if(child)
      param.materialOutputDir = child->GetText() ? child->GetText() : "";
    
    child = rootElem->FirstChildElement("IDC_TEXDIR");
    if(child)
      param.texOutputDir = child->GetText() ? child->GetText() : "";

    child = rootElem->FirstChildElement("IDC_MESHDIR");
    if(child)
      param.meshOutputDir = child->GetText() ? child->GetText() : "";

    child = rootElem->FirstChildElement("IDC_PROGDIR");
    if(child)
      param.programOutputDir = child->GetText() ? child->GetText() : "";

    child = rootElem->FirstChildElement("IDC_YUPAXIS");
    if(child)
      param.yUpAxis = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_SHAREDGEOM");
    if(child)
      param.useSharedGeom = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_GENLOD");
    if(child)
      param.generateLOD = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_EDGELIST");
    if(child)
      param.buildEdges = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_TANGENT");
    if(child)
      param.buildTangents = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_SPLITMIRROR");
    if(child)
      param.tangentsSplitMirrored = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_SPLITROT");
    if(child)
      param.tangentsSplitRotated = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_STOREPARITY");
    if(child)
      param.tangentsUseParity = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_CONVDDS");
    if(child)
      param.convertToDDS = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_RESAMPLE_ANIMS");
    if(child)
      param.resampleAnims = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_RESAMPLE_STEP");
    if(child)
      param.resampleStep = (child->GetText()) ? atoi(child->GetText()) : 1;

    child = rootElem->FirstChildElement("IDC_LOGS");
    if (child)
      param.enableLogs = (child->GetText() && (atoi(child->GetText()) == 1)) ? true : false;

    child = rootElem->FirstChildElement("IDC_SHADERMODE");
    if(child)
    {
      if(child->GetText())
      {
        switch (atoi(child->GetText()))
        {
          case 0:
            param.exportProgram = SHADER_NONE;
            break;
          case 1:
            param.exportProgram = SHADER_BUMP;
            break;
          case 2:
            param.exportProgram = SHADER_ALL;
            break;

            /*
          case 3:
            param.exportProgram = SHADER_ALL_MULTI;
            break;
            */

          default:
            param.exportProgram = SHADER_ALL;
        }
      }
    }

    child = rootElem->FirstChildElement("IDC_TEXSIZE");
    if(child && child->GetText())
      param.maxTextureSize = atoi(child->GetText());

    child = rootElem->FirstChildElement("IDC_NUMMIPS");
    if(child && child->GetText())
      param.maxMipmaps = atoi(child->GetText());
  }
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

void OgreExporter::saveExportConf(std::string path)
{
  TiXmlDocument xmlDoc;
  TiXmlElement* contProperties = new TiXmlElement("Config");
  xmlDoc.LinkEndChild(contProperties);

  std::stringstream oVersionVal;
  oVersionVal << m_params.meshVersion;
  TiXmlElement* child = new TiXmlElement("IDC_OGREVERSION");
  TiXmlText* childText = new TiXmlText(oVersionVal.str().c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_RESPREFIX");
  childText = new TiXmlText(m_params.resPrefix.c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_MATDIR");
  childText = new TiXmlText(m_params.materialOutputDir.c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_TEXDIR");
  childText = new TiXmlText(m_params.texOutputDir.c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);
    
  child = new TiXmlElement("IDC_MESHDIR");
  childText = new TiXmlText(m_params.meshOutputDir.c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_PROGDIR");
  childText = new TiXmlText(m_params.programOutputDir.c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_YUPAXIS");
  childText = new TiXmlText(m_params.yUpAxis ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_SHAREDGEOM");
  childText = new TiXmlText(m_params.useSharedGeom ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_GENLOD");
  childText = new TiXmlText(m_params.generateLOD ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_EDGELIST");
  childText = new TiXmlText(m_params.buildEdges ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_TANGENT");
  childText = new TiXmlText(m_params.buildTangents ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_SPLITMIRROR");
  childText = new TiXmlText(m_params.tangentsSplitMirrored ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_SPLITROT");
  childText = new TiXmlText(m_params.tangentsSplitRotated ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_STOREPARITY");
  childText = new TiXmlText(m_params.tangentsUseParity ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_RESAMPLE_ANIMS");
  childText = new TiXmlText(m_params.resampleAnims ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_RESAMPLE_STEP");
  char tmpbuf[10] = {0};
  sprintf(tmpbuf, "%d", m_params.resampleStep);
  childText = new TiXmlText(tmpbuf);
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_LOGS");
  childText = new TiXmlText(m_params.enableLogs ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  child = new TiXmlElement("IDC_CONVDDS");
  childText = new TiXmlText(m_params.convertToDDS ? "1" : "0");
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  std::stringstream oShaderVal;
  oShaderVal << m_params.exportProgram;
  child = new TiXmlElement("IDC_SHADERMODE");
  childText = new TiXmlText(oShaderVal.str().c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  std::stringstream oTexVal;
  oTexVal << m_params.maxTextureSize;
  child = new TiXmlElement("IDC_TEXSIZE");
  childText = new TiXmlText(oTexVal.str().c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  std::stringstream oMipsVal;
  oMipsVal << m_params.maxMipmaps;
  child = new TiXmlElement("IDC_NUMMIPS");
  childText = new TiXmlText(oMipsVal.str().c_str());
  child->LinkEndChild(childText);
  contProperties->LinkEndChild(child);

  xmlDoc.SaveFile(path.c_str());
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

  // userData
  TiXmlElement* userData = new TiXmlElement("UserProperty");
  TiXmlElement* userDataId = new TiXmlElement("id");
  TiXmlText* userDataIdText = new TiXmlText("104");
  userDataId->LinkEndChild(userDataIdText);
  userData->LinkEndChild(userDataId);
  
  TiXmlElement* userDataSName = new TiXmlElement("simplename");
  TiXmlText* userDataSNameText = new TiXmlText("userData");
  userDataSName->LinkEndChild(userDataSNameText);
  userData->LinkEndChild(userDataSName);

  TiXmlElement* userDataName = new TiXmlElement("keyName");
  TiXmlText* userDataNameText = new TiXmlText("userData");
  userDataName->LinkEndChild(userDataNameText);
  userData->LinkEndChild(userDataName);

  TiXmlElement* userDataType = new TiXmlElement("type");
  TiXmlText* userDataTypeText = new TiXmlText("string");
  userDataType->LinkEndChild(userDataTypeText);
  userData->LinkEndChild(userDataType);

  igameUserData->LinkEndChild(userData);

  xmlDoc.SaveFile(path.c_str());
}

bool OgreExporter::exportScene()
{
  nodeCount = 0;

  //Init log files
  std::string logFileName = "";
  if (m_params.enableLogs)
  {
    logFileName = m_params.outputDir;
    logFileName.append("\\easyOgreExporter.log");
    EasyOgreExporter::EasyOgreExporterLogFile::SetPath(logFileName.data());
  }
  else
  {
    EasyOgreExporter::EasyOgreExporterLogFile::SetPath(logFileName.data());
  }


#if defined(WIN32)
  if(m_params.exportMaterial)
    _mkdir((makeOutputPath(m_params.outputDir, m_params.materialOutputDir, "", "")).c_str());
 
  if(m_params.exportProgram != SHADER_NONE)
    _mkdir((makeOutputPath(m_params.outputDir, m_params.programOutputDir, "", "")).c_str());

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
  Ogre::OldSkeletonManager skelMgr;
  Ogre::MaterialManager matMgr;
  Ogre::DefaultHardwareBufferManager hardwareBufMgr;
  Ogre::LodStrategyManager lodstrategymanager;  

  m_params.currentRootJoints.clear();

#ifdef UNICODE
  std::wstring plugConfDir_w = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
  std::string plugConfDir;
  plugConfDir.assign(plugConfDir_w.begin(),plugConfDir_w.end());
#else
  std::string plugConfDir = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
#endif

  _mkdir((std::string(plugConfDir + "\\EasyOgreExporter")).c_str());

  std::string xmlIGameConfPath = plugConfDir + "\\EasyOgreExporter\\IGameProp.xml";
  initIGameConf(xmlIGameConfPath);

  std::string xmlConfPath = plugConfDir + "\\EasyOgreExporter\\config.xml";
  saveExportConf(xmlConfPath);

  pIGame = GetIGameInterface();
#ifdef UNICODE
  std::wstring xmlIGameConfPath_w; 
  xmlIGameConfPath_w.assign(xmlIGameConfPath.begin(),xmlIGameConfPath.end());
  pIGame->SetPropertyFile(xmlIGameConfPath_w.data());
#else
  pIGame->SetPropertyFile(xmlIGameConfPath.c_str());
#endif

  // Passing in true causing crash on IGameNode->GetNodeParent.  
  // Test for selection in Translate node.
  pIGame->InitialiseIGame(false);
  pIGame->SetStaticFrame(0);

  std::string sTitle = "Easy Ogre Exporter: " + m_params.sceneFilename;
#ifdef UNICODE
  std::wstring title_w;
  title_w.assign(sTitle.begin(),sTitle.end());
  GetCOREInterface()->ProgressStart(title_w.data(), TRUE, progressCb, 0);
#else
  GetCOREInterface()->ProgressStart((char*)sTitle.c_str(), TRUE, progressCb, 0);
#endif

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

  ogreConverter = new ExOgreConverter(pIGame, m_params);

  //Init Scene
  if(m_params.exportScene)
    sceneData = new ExScene(ogreConverter);

  // parse Max scene to find bones used in skins
  for(int node = 0; node < pIGame->GetTopLevelNodeCount(); ++node)
  {
    IGameNode* pGameNode = pIGame->GetTopLevelNode(node);
    if(pGameNode)
    {
      LoadSkinBones(pGameNode);
    }
  }

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
#ifdef UNICODE
    GetCOREInterface()->ProgressUpdate(95, FALSE, L"Writing scene file.");
#else
    GetCOREInterface()->ProgressUpdate(95, FALSE, "Writing scene file.");
#endif
    sceneData->writeSceneFile();
  }

  if (ogreConverter)
  {
#ifdef UNICODE
    GetCOREInterface()->ProgressUpdate(98, FALSE, L"Writing material file.");
#else
    GetCOREInterface()->ProgressUpdate(98, FALSE, "Writing material file.");
#endif
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
  
#ifdef UNICODE
  GetCOREInterface()->ProgressUpdate(99, FALSE, L"Done.");
#else
  GetCOREInterface()->ProgressUpdate(99, FALSE, "Done.");
#endif
  EasyOgreExporterLog("Info: export done.\n");

  //close the progress bar
  GetCOREInterface()->ProgressEnd();
  lFoundBones.clear();

  MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Export done successfully."), _T("Info"), MB_OK);
  return true;
}

void OgreExporter::LoadSkinBones(IGameNode* pGameNode)
{
  IGameObject* pGameObject = pGameNode->GetIGameObject();
  if(pGameObject)
  {
    IGameObject::ObjectTypes gameType = pGameObject->GetIGameType();

    if (gameType == IGameObject::IGAME_MESH)
    {
      IGameMesh* pGameMesh = static_cast<IGameMesh*>(pGameObject);
      if(pGameMesh)
      {
        //search skin modifier
        int numModifiers = pGameMesh->GetNumModifiers();
        for(int i = 0; i < numModifiers; ++i)
        {
          IGameModifier* pGameModifier = pGameMesh->GetIGameModifier(i);
          if(pGameModifier)
          {
            if(pGameModifier->IsSkin())
            {
              IGameSkin* pGameSkin = static_cast<IGameSkin*>(pGameModifier);
              for(int j = 0; j < pGameSkin->GetTotalBoneCount(); ++j)
              {
                INode* bone = pGameSkin->GetBone(j, true);
                lFoundBones.push_back(bone);
              }

              ogreConverter->addSkinModifier(pGameSkin, pGameNode);
            }
          }
        }
      }
    }

    pGameNode->ReleaseIGameObject();
  }

  for(int i = 0; i < pGameNode->GetChildCount(); ++i)
  {
    IGameNode* pChildGameNode = pGameNode->GetNodeChild(i);
    if(pChildGameNode)
    {
      LoadSkinBones(pChildGameNode);
    }
  }
}

bool OgreExporter::IsSkinnedBone(IGameNode* pGameNode)
{
  INode* node = pGameNode->GetMaxNode();
  for (unsigned int i = 0; i < lFoundBones.size(); i++)
  {
    if (lFoundBones[i] == node)
      return true;
  }

  return false;
}

bool OgreExporter::IsNodeToExport(IGameNode* pGameNode)
{
  bool bShouldExport = false;

  if(pGameNode)
  {
    IGameObject* pGameObject = pGameNode->GetIGameObject();
    if(pGameObject)
    {
      IGameObject::ObjectTypes gameType = pGameObject->GetIGameType();
      IGameObject::MaxType maxType = pGameObject->GetMaxType();
      INode* node = pGameNode->GetMaxNode();
      if(node)
      {
        ILayer* layer = (ILayer*)node->GetReference(NODE_LAYER_REF);
       
        if(node->IsObjectHidden() || layer->IsHidden()
          // Only export selection if exportAll = false
          || (node->Selected() == 0 && !m_params.exportAll)
          // Do not export bones
          || node->GetBoneNodeOnOff() || IsSkinnedBone(pGameNode)
          || (maxType == IGameObject::IGAME_MAX_UNKNOWN || maxType == IGameObject::IGAME_MAX_BONE ||
            gameType == IGameObject::IGAME_BONE || gameType == IGameObject::IGAME_IKCHAIN ||
            gameType == IGameObject::IGAME_UNKNOWN))
        {
          bShouldExport = false;
        }
        else
        {
          bShouldExport = true;
        }
      }
      pGameNode->ReleaseIGameObject();
    }
  }
  return bShouldExport;
}

bool OgreExporter::exportNode(IGameNode* pGameNode, TiXmlElement* parent)
{
  GetCOREInterface()->ProgressUpdate((int)(((float)nodeCount / (float)pIGame->GetTotalNodeCount()) * 90.0f), TRUE); 

  if(IsNodeToExport(pGameNode))
  {
    GetCOREInterface()->ProgressUpdate((int)(((float)nodeCount / (float)pIGame->GetTotalNodeCount()) * 90.0f), FALSE, pGameNode->GetName());

    #ifdef UNICODE
      MSTR nodeName = pGameNode->GetName();
      EasyOgreExporterLog("Found node: %ls\n", nodeName.data());
    #else
      EasyOgreExporterLog("Found node: %s\n", pGameNode->GetName());
    #endif

    IGameObject* pGameObject = pGameNode->GetIGameObject();
    if(pGameObject)
    {
      IGameObject::ObjectTypes gameType = pGameObject->GetIGameType();
        
      switch(gameType)
      {
        case IGameObject::IGAME_UNKNOWN:
          {
#ifdef UNICODE
          MSTR nodeName = pGameNode->GetName();
          EasyOgreExporterLog("Warning, Object type unknown : %ls try for mesh\n", nodeName.data());
#else
          EasyOgreExporterLog("Warning, Object type unknown: %s try for mesh\n", pGameNode->GetName());
#endif
          }
        case IGameObject::IGAME_MESH:
          {
            bool delTri = false;
            Mesh* mMesh = 0;
            INode* node = pGameNode->GetMaxNode();
            TriObject* triObj = getTriObjectFromNode(node, GetFirstFrame(), delTri);

            if (triObj)
              mMesh = &triObj->GetMesh();

            int numFaces = 0;
            if(mMesh)
            {
              numFaces = mMesh->getNumFaces();

              //free the tree object
              if(delTri && triObj)
              {
                triObj->DeleteThis();
                triObj = 0;
                delTri = false;
              }
            }

            IGameMesh* pGameMesh = static_cast<IGameMesh*>(pGameObject);
            if(pGameMesh && (numFaces > 0))
            {
              if(pGameMesh->InitializeData())
              {
                #ifdef UNICODE
                  MSTR nodeName = pGameNode->GetName();
                  EasyOgreExporterLog("Found mesh node: %ls\n", nodeName.data());
                #else
                  EasyOgreExporterLog("Found mesh node: %s\n", pGameNode->GetName());
                #endif

                std::vector<ExMaterial*> lmat;
                if(ogreConverter)
                {
                  if (!ogreConverter->writeEntityData(pGameNode, pGameObject, pGameMesh, lmat))
                  {
                    EasyOgreExporterLog("Warning, mesh skipped\n");
                  }
                  else if (sceneData)
                  {
                    parent = sceneData->writeNodeData(parent, pGameNode, IGameObject::IGAME_MESH);
                    sceneData->writeEntityData(parent, pGameNode, pGameMesh, lmat);
                  }
                }
              }
            }
            else
            {
              #ifdef UNICODE
                MSTR nodeName = pGameNode->GetName();
                EasyOgreExporterLog("Warning, Mesh : %ls skipped, no faces found\n", nodeName.data());
              #else
                EasyOgreExporterLog("Warning, Mesh node: %s skipped, no faces found\n", pGameNode->GetName());
              #endif
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
              #ifdef UNICODE
                MSTR nodeName = pGameNode->GetName();
                EasyOgreExporterLog("Found light: %ls\n", nodeName.data());
              #else
                EasyOgreExporterLog("Found light: %s\n", pGameNode->GetName());
              #endif
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
              #ifdef UNICODE
                MSTR nodeName = pGameNode->GetName();
                EasyOgreExporterLog("Found camera: %ls\n", nodeName.data());
              #else
                EasyOgreExporterLog("Found camera: %s\n", pGameNode->GetName());
              #endif

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
  }

  if(pGameNode)
  {
    for(int i = 0; i < pGameNode->GetChildCount(); ++i)
    {
      IGameNode* pChildGameNode = pGameNode->GetNodeChild(i);
      if(pChildGameNode)
      {
        exportNode(pChildGameNode, parent);
      }
    }
  }

  pGameNode->ReleaseIGameObject();

  nodeCount++;

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

