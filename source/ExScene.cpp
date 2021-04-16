////////////////////////////////////////////////////////////////////////////////
// ExScene.cpp
// Author     : Bastien Bourineau
// Start Date : Junary 11th, 2012
// Copyright  : Copyright (c) 2011 OpenSpace3D
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
#include "ExMaterial.h"
#include "ExScene.h"
#include "EasyOgreExporterLog.h"
#include "ExTools.h"
#include "decomp.h"
#include "IFrameTagManager.h"

namespace EasyOgreExporter
{
	// constructor
	ExScene::ExScene(ExOgreConverter* converter)
	{
		id_counter = 0;
    m_converter = converter;
    ParamList mParams = converter->getParams();

    //TODO test directories ?
    scenePath = makeOutputPath(mParams.outputDir, "", mParams.sceneFilename, "scene");

    initXmlDocument();
	}

	// destructor
	ExScene::~ExScene()
	{
    if (xmlDoc != 0)
      delete(xmlDoc);
	}

  void ExScene::initXmlDocument()
  {
    ParamList mParams = m_converter->getParams();

    xmlDoc = new TiXmlDocument();
    sceneElement = new TiXmlElement("scene");
    xmlDoc->LinkEndChild(sceneElement);

    //TODO Ogre version, App Name, units
    sceneElement->SetAttribute("upAxis", mParams.yUpAxis ? "y" : "z");
    sceneElement->SetAttribute("unitsPerMeter", "1");
    sceneElement->SetAttribute("unitType", "meters");
    sceneElement->SetAttribute("formatVersion", "1.0");
    sceneElement->SetAttribute("minOgreVersion", mParams.getOgreVersionName().c_str());
		sceneElement->SetAttribute("author", "EasyOgreExporter");

    //Environment setting
    TiXmlElement* envElement = new TiXmlElement("environment");
    sceneElement->LinkEndChild(envElement);

    Point3 ambColor = GetCOREInterface()->GetAmbient(0, FOREVER);
    TiXmlElement* ambElement = new TiXmlElement("colourAmbient");
    ambElement->SetDoubleAttribute("r", ambColor.x);
	  ambElement->SetDoubleAttribute("g", ambColor.y);
	  ambElement->SetDoubleAttribute("b", ambColor.z);
    envElement->LinkEndChild(ambElement);
    
    Point3 bgColor = GetCOREInterface()->GetBackGround(0, FOREVER);
    TiXmlElement* bgElement = new TiXmlElement("colourBackground");
    bgElement->SetDoubleAttribute("r", bgColor.x);
	  bgElement->SetDoubleAttribute("g", bgColor.y);
	  bgElement->SetDoubleAttribute("b", bgColor.z);
    envElement->LinkEndChild(bgElement);

		nodesElement = new TiXmlElement("nodes");
		sceneElement->LinkEndChild(nodesElement);
  }

  std::string ExScene::getBoolString(bool value)
  {
    return value ? std::string("true") : std::string("false");
  }

	std::string ExScene::getLightTypeString(ExOgreLightType type)
	{
		switch(type)
		{
		case OGRE_LIGHT_POINT:
			return std::string("point");
		case OGRE_LIGHT_DIRECTIONAL:
			return std::string("directional");
		case OGRE_LIGHT_SPOT:
			return std::string("spot");
		case OGRE_LIGHT_RADPOINT:
			return std::string("radpoint");
		}
		EasyOgreExporterLog("Invalid light type detected. Using point light as default.\n");
		return("point");
	}

  bool ExScene::exportNodeAnimation(TiXmlElement* pAnimsElement, IGameNode* pGameNode, Interval animRange, std::string name, bool resample, IGameObject::ObjectTypes type)
  {
    ParamList mParams = m_converter->getParams();
    std::vector<int> animKeys = GetAnimationsKeysTime(pGameNode, animRange, resample, mParams.resampleStep);
    INode* maxnode = pGameNode->GetMaxNode();
    int firstFrame = GetFirstFrame();

    //optimize the animation
    OptimizeNodeAnimation(maxnode, animKeys);

    if(animKeys.size() > 0)
    {
      //look if any key change something before export
      bool isAnimated = false;
      Matrix3 prevKeyTM = GetLocalNodeMatrix(maxnode, mParams.yUpAxis, firstFrame);
      for(int i = 0; i < animKeys.size() && !isAnimated; i++)
      {
        // get the relative transform
        Matrix3 keyTM = GetLocalNodeMatrix(maxnode, mParams.yUpAxis, animKeys[i]);
        
        if(keyTM.Equals(prevKeyTM) == 0)
          isAnimated = true;

        prevKeyTM = keyTM;
      }
      prevKeyTM.IdentityMatrix();

      if(!isAnimated)
        return false;

      TimeValue length = animRange.End() - animRange.Start();
      float ogreAnimLength = (static_cast<float>(length) / static_cast<float>(GetTicksPerFrame())) / GetFrameRate();

      TiXmlElement* pAnimElement = new TiXmlElement("animation");
      pAnimElement->SetAttribute("name", name.c_str());
      pAnimElement->SetAttribute("enable", "false");
      pAnimElement->SetAttribute("loop", "false");
      pAnimElement->SetAttribute("interpolationMode", "linear");
      pAnimElement->SetAttribute("rotationInterpolationMode", "linear");
      pAnimElement->SetDoubleAttribute("length", ogreAnimLength);
      pAnimsElement->LinkEndChild(pAnimElement);
      
      for(int i = 0; i < animKeys.size(); i++)
      {        
        // get the relative transform
        Matrix3 keyTM = GetLocalNodeMatrix(maxnode, mParams.yUpAxis, animKeys[i]);

        //remove only key if the next key is not different
        Matrix3 nextKeyTM;
        nextKeyTM.IdentityMatrix();
        if(i + 1 < animKeys.size())
          nextKeyTM = GetLocalNodeMatrix(maxnode, mParams.yUpAxis, animKeys[i + 1]);
        
        //skip if the key is equal to the last one
        if(i > 0)
        {
          if(keyTM.Equals(prevKeyTM) && (keyTM.Equals(nextKeyTM)))
            continue;

          prevKeyTM = keyTM;
        }

        float ogreTime = (static_cast<float>(animKeys[i] - animRange.Start()) / static_cast<float>(GetTicksPerFrame())) / GetFrameRate();

        AffineParts apKey;
        decomp_affine(keyTM, &apKey);

        Point3 trans = apKey.t * mParams.lum;
        Point3 scale = apKey.k;
        Quat rot = apKey.q;

        if((type == IGameObject::IGAME_CAMERA) && mParams.yUpAxis)
        {
          // Now rotate around the X Axis PI/2
          Quat zRev = RotateXMatrix(PI/2);
          rot = rot / zRev;
        }
        
        if((type == IGameObject::IGAME_LIGHT) && mParams.yUpAxis)
        {
          // Now rotate around the X Axis -PI/2
          Quat zRev = RotateXMatrix(-PI/2);
          rot = rot / zRev;
        }

        // Notice that in Max we flip the w-component of the quaternion;
        rot.w = -rot.w;
        rot.Normalize();

        TiXmlElement* pKeyElement = new TiXmlElement("keyframe");
        pKeyElement->SetDoubleAttribute("time", ogreTime);
        pAnimElement->LinkEndChild(pKeyElement);

        TiXmlElement* pKeyTransElement = new TiXmlElement("position");
        pKeyTransElement->SetDoubleAttribute("x", trans.x);
        pKeyTransElement->SetDoubleAttribute("y", trans.y);
        pKeyTransElement->SetDoubleAttribute("z", trans.z);
        pKeyElement->LinkEndChild(pKeyTransElement);

        TiXmlElement* pKeyRotElement = new TiXmlElement("rotation");
        pKeyRotElement->SetDoubleAttribute("qx", rot.x);
        pKeyRotElement->SetDoubleAttribute("qy", rot.y);
        pKeyRotElement->SetDoubleAttribute("qz", rot.z);
        pKeyRotElement->SetDoubleAttribute("qw", rot.w);
        pKeyElement->LinkEndChild(pKeyRotElement);

        TiXmlElement* pKeyScaleElement = new TiXmlElement("scale");
        pKeyScaleElement->SetDoubleAttribute("x", scale.x);
        pKeyScaleElement->SetDoubleAttribute("y", scale.y);
        pKeyScaleElement->SetDoubleAttribute("z", scale.z);
        pKeyElement->LinkEndChild(pKeyScaleElement);
      }

      animKeys.clear();
      return true;
    }

    return false;
  }

	TiXmlElement* ExScene::writeNodeData(TiXmlElement* parent, IGameNode* pGameNode, IGameObject::ObjectTypes type)
	{
    ParamList mParams = m_converter->getParams();

		if(!parent)
      parent = nodesElement;
        
    INode* maxnode = pGameNode->GetMaxNode();
    
    // get the relative transform
    Matrix3 nodeTM = GetLocalNodeMatrix(maxnode, mParams.yUpAxis, GetFirstFrame());

    AffineParts ap;
    decomp_affine(nodeTM, &ap);

    Point3 trans = ap.t * mParams.lum;
    Point3 scale = ap.k;
    Quat rot = ap.q;

    if((type == IGameObject::IGAME_CAMERA) && mParams.yUpAxis)
    {
      // Now rotate around the X Axis PI/2
      Quat zRev = RotateXMatrix(PI/2);
      rot = rot / zRev;
    }
    
    if((type == IGameObject::IGAME_LIGHT) && mParams.yUpAxis)
    {
      // Now rotate around the X Axis -PI/2
      Quat zRev = RotateXMatrix(-PI/2);
      rot = rot / zRev;
    }

    // Notice that in Max we flip the w-component of the quaternion;
    rot.w = -rot.w;
    rot.Normalize();

		TiXmlElement* pNodeElement = new TiXmlElement("node");
#ifdef UNICODE
		std::wstring name_w = pGameNode->GetName();
		std::string name_s = mParams.resPrefix;
		name_s.append(name_w.begin(),name_w.end());
		pNodeElement->SetAttribute("name", optimizeResourceName(name_s).c_str());
#else
		std::string name = mParams.resPrefix;
		name.append(pGameNode->GetName());
		pNodeElement->SetAttribute("name", optimizeResourceName(name).c_str());
#endif
		pNodeElement->SetAttribute("id", id_counter);
		pNodeElement->SetAttribute("isTarget", "false");
		parent->LinkEndChild(pNodeElement);
 
		TiXmlElement* pPositionElement = new TiXmlElement("position");
    pPositionElement->SetDoubleAttribute("x", trans.x);
		pPositionElement->SetDoubleAttribute("y", trans.y);
		pPositionElement->SetDoubleAttribute("z", trans.z);
		pNodeElement->LinkEndChild(pPositionElement);

		TiXmlElement* pRotationElement = new TiXmlElement("rotation");
		pRotationElement->SetDoubleAttribute("qx", rot.x);
		pRotationElement->SetDoubleAttribute("qy", rot.y);
		pRotationElement->SetDoubleAttribute("qz", rot.z);
		// Notice that in Max we flip the w-component of the quaternion;
		pRotationElement->SetDoubleAttribute("qw", rot.w);
		pNodeElement->LinkEndChild(pRotationElement);

		TiXmlElement* pScaleElement = new TiXmlElement("scale");
		pScaleElement->SetDoubleAttribute("x", scale.x);
		pScaleElement->SetDoubleAttribute("y", scale.y);
		pScaleElement->SetDoubleAttribute("z", scale.z);
		pNodeElement->LinkEndChild(pScaleElement);

    //node animations
    IGameControl* nodeControl = pGameNode->GetIGameControl();

    //try to get animations in motion mixer
    bool useDefault = true;
    IMixer8* mixer = TheMaxMixerManager.GetMaxMixer(maxnode);
    if(mixer || nodeControl->IsAnimated(IGAME_POS) || nodeControl->IsAnimated(IGAME_ROT) || nodeControl->IsAnimated(IGAME_SCALE))
    {
      //Add the default animation and the track
      TiXmlElement* pAnimsElement = new TiXmlElement("animations");
      pNodeElement->LinkEndChild(pAnimsElement);

      if(mixer)
      {
        int clipId = 0;
        int numGroups = mixer->NumTrackgroups();
        for (size_t j = 0; j < numGroups; j++)
        {
          IMXtrackgroup* group = mixer->GetTrackgroup(j);
          
          #ifdef UNICODE
            EasyOgreExporterLog("Info : mixer track found %ls\n", group->GetName());
          #else
            EasyOgreExporterLog("Info : mixer track found %s\n", group->GetName());
          #endif

          int numTracks = group->NumTracks();
          for (size_t k = 0; k < numTracks; k++)
          {
            IMXtrack* track = group->GetTrack(k);
            BOOL tMode = track->GetSolo();
            track->SetSolo(TRUE);

            int numClips = track->NumClips(BOT_ROW);
            for (size_t l = 0; l < numClips; l++)
            {
              IMXclip* clip = track->GetClip(l, BOT_ROW);
              if(clip)
              {
                Interval animRange;
                int start;
                int stop;
                #ifdef PRE_MAX_2010
                  std::string clipName = formatClipName(std::string(clip->GetFilename()), clipId);
                #else
                MaxSDK::AssetManagement::AssetUser &clipFile = const_cast<MaxSDK::AssetManagement::AssetUser&>(clip->GetFile());
#ifdef UNICODE
				std::wstring clipFileName_w = clipFile.GetFileName();
				std::string clipFileName_s;
				clipFileName_s.assign(clipFileName_w.begin(),clipFileName_w.end());
				std::string clipName = formatClipName(clipFileName_s, clipId);
#else
                  std::string clipName = formatClipName(std::string(clipFile.GetFileName()), clipId);
#endif
                #endif

                clip->GetGlobalBounds(&start, &stop);
                animRange.SetStart(start);
                animRange.SetEnd(stop);
                EasyOgreExporterLog("Info : mixer clip found %s from %i to %i\n", clipName.c_str(), start, stop);
                
                if(exportNodeAnimation(pAnimsElement, pGameNode, animRange, clipName, mParams.resampleAnims, type))
                  useDefault = false;
                
                clipId++;
              }
            }
            track->SetSolo(tMode);
          }
        }
      }

      if(useDefault)
      {
        Interval animRange = GetCOREInterface()->GetAnimRange();
        IFrameTagManager* frameTagMgr = static_cast<IFrameTagManager*>(GetCOREInterface(FRAMETAGMANAGER_INTERFACE));
        int cnt = frameTagMgr->GetTagCount();

        if(!cnt)
        {
          exportNodeAnimation(pAnimsElement, pGameNode, animRange, "default", mParams.resampleAnims, type);
        }
        else
        {
          for(int i = 0; i < cnt; i++)
          {
            DWORD t = frameTagMgr->GetTagID(i);
            DWORD tlock = frameTagMgr->GetLockIDByID(t);
            
            //ignore locked tags used for animation end
            if(tlock != 0)
              continue;

            TimeValue tv = frameTagMgr->GetTimeByID(t, FALSE);
            TimeValue te = animRange.End();
            
            DWORD tnext = 0;
            if((i + 1) < cnt)
            {
              tnext = frameTagMgr->GetTagID(i + 1);
              te = frameTagMgr->GetTimeByID(tnext, FALSE);
            }

            Interval ianim(tv, te);
#ifdef UNICODE
			std::wstring name_w = frameTagMgr->GetNameByID(t);
			std::string name_s;
			name_s.assign(name_w.begin(), name_w.end());
            exportNodeAnimation(pAnimsElement, pGameNode, ianim, std::string(name_s), mParams.resampleAnims, type);
#else
            exportNodeAnimation(pAnimsElement, pGameNode, ianim, std::string(frameTagMgr->GetNameByID(t)), mParams.resampleAnims, type);
#endif
          }
        }
      }
    }

    id_counter++;
		return pNodeElement;
	}

	TiXmlElement* ExScene::writeEntityData(TiXmlElement* parent, IGameNode* pGameNode, IGameMesh* pGameMesh, std::vector<ExMaterial*> lmat)
	{
    ParamList mParams = m_converter->getParams();
    
		if(!parent)
      return 0;
    
    if (!pGameMesh->IsEntitySupported())
    {
      EasyOgreExporterLog("Unsupported mesh type. Failed to export %s.\n", parent->Attribute("name"));
      return 0;
    }

    /*if (pGameMesh->GetNumberOfVerts() == 0)
    {
      EasyOgreExporterLog("Bad vertex count (0). Failed to export %s.\n", parent->Attribute("name"));
      return 0;
    }*/

    //object user params
    float renderDistance = 0.0f;
    IPropertyContainer* pc = pGameMesh->GetIPropertyContainer();
    IGameProperty* pRenderDistance = pc->QueryProperty(_T("renderingDistance"));
    if(pRenderDistance)
      pRenderDistance->GetPropertyValue(renderDistance);

    std::string entityName = optimizeResourceName(parent->Attribute("name"));

    TiXmlElement* pEntityElement = new TiXmlElement("entity");
    pEntityElement->SetAttribute("name", entityName.c_str());
    pEntityElement->SetAttribute("id", id_counter);

    std::string instName = mParams.resPrefix;
    instName.append(getFirstInstanceName(pGameNode));
    instName = optimizeResourceName(instName);
    
    std::string meshPath = optimizeFileName(instName + ".mesh");

    pEntityElement->SetAttribute("meshFile", meshPath.c_str());
    pEntityElement->SetAttribute("castShadows", getBoolString(pGameMesh->CastShadows()).c_str());
    if(renderDistance != 0.0f)
      pEntityElement->SetDoubleAttribute("renderingDistance", renderDistance);

    parent->LinkEndChild(pEntityElement);

    // user Data
    IPropertyContainer* upc = pGameMesh->GetIPropertyContainer();
    IGameProperty* pUserData = upc->QueryProperty(_T("userData"));
    if(pUserData)
    {
      TiXmlElement* pUserDataElement = new TiXmlElement("userData");
      TiXmlText* pUserDataText = 0;

#ifdef UNICODE
      const MCHAR* userData = 0;
      pUserData->GetPropertyValue(userData);
      std::wstring userData_w = userData;

      std::string userData_s;
		  userData_s.assign(userData_w.begin(), userData_w.end());
      pUserDataText = new TiXmlText(userData_s.c_str());
#else
      #ifdef PRE_MAX_2010
      char* userData = 0;
      #else
      const char* userData = 0;
      #endif
      pUserData->GetPropertyValue(userData);
      pUserDataText = new TiXmlText(userData);
#endif

      pUserDataElement->LinkEndChild(pUserDataText);
      pEntityElement->LinkEndChild(pUserDataElement);
    }

    TiXmlElement* pSubEntities = new TiXmlElement("subentities");
    pEntityElement->LinkEndChild(pSubEntities);

    for (int i = 0; i < lmat.size(); i++)
    {
      TiXmlElement* pSubElement = new TiXmlElement("subentity");
      pSubElement->SetAttribute("index", i);
      pSubElement->SetAttribute("materialName", lmat[i]->getName().c_str());
      pSubEntities->LinkEndChild(pSubElement);
    }

    id_counter++;
		return pEntityElement;
	}

  TiXmlElement* ExScene::writeCameraData(TiXmlElement* parent, IGameCamera* pGameCamera)
  {
    ParamList mParams = m_converter->getParams();

		if(!parent)
      return 0;

    if (!pGameCamera->IsEntitySupported())
    {
      EasyOgreExporterLog("Unsupported camera type. Failed to export.\n");
      return 0;
    }

    TiXmlElement* pCameraElement = new TiXmlElement("camera");
		pCameraElement->SetAttribute("name", parent->Attribute("name"));
		pCameraElement->SetAttribute("id", id_counter);

    //default 45�
    float camFov = 0.785398163f;
    IGameProperty* pGameProperty = pGameCamera->GetCameraFOV();
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(camFov);
    }
    pCameraElement->SetDoubleAttribute("fov", camFov);
    parent->LinkEndChild(pCameraElement);

    float neerClip = 0.01f;
    pGameProperty = pGameCamera->GetCameraNearClip ();
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(neerClip);
    }
    neerClip = neerClip * mParams.lum;

    float farClip = 1000.0f;
    pGameProperty = pGameCamera->GetCameraFarClip();
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(farClip);
    }
    farClip = farClip * mParams.lum;

		TiXmlElement* pClippingElement = new TiXmlElement("clipping");
		pClippingElement->SetDoubleAttribute("near", neerClip);
		pClippingElement->SetDoubleAttribute("far", farClip);
		pCameraElement->LinkEndChild(pClippingElement);

    id_counter++;
		return pCameraElement;
  }

  TiXmlElement* ExScene::writeLightData(TiXmlElement* parent, IGameLight* pGameLight)
  {
    ParamList mParams = m_converter->getParams();

		if(!parent)
      return 0;
    
    if (!pGameLight->IsEntitySupported())
    {
      EasyOgreExporterLog("Unsupported light type. Failed to export.\n");
      return 0;
    }

#ifdef UNICODE
	std::wstring lightClass_w = pGameLight->GetClassName();
    std::string lightClass;
	lightClass.assign(lightClass_w.begin(), lightClass_w.end());
#else
    std::string lightClass = pGameLight->GetClassName();
#endif
    if(lightClass == "Missing Light")
    {
      EasyOgreExporterLog("Unsupported light type. Failed to export.\n");
      return 0;
    }

    ExOgreLightType lightType = OGRE_LIGHT_DIRECTIONAL;
    switch(pGameLight->GetLightType())
    {
      case IGameLight::IGAME_OMNI:
        lightType = OGRE_LIGHT_POINT;
        break;
      case IGameLight::IGAME_DIR:
        lightType = OGRE_LIGHT_DIRECTIONAL;
        break;
      case IGameLight::IGAME_FSPOT:
        lightType = OGRE_LIGHT_SPOT;
        break;
      // No support for targeted lights at the moment, but
      // We can still export them facing the initial direction.
      case IGameLight::IGAME_TSPOT:
        lightType = OGRE_LIGHT_SPOT;
        break;
      case IGameLight::IGAME_TDIR:
        lightType = OGRE_LIGHT_DIRECTIONAL;
        break;
      case IGameLight::IGAME_UNKNOWN:
      default:
        EasyOgreExporterLog("Unsupported light type. Failed to export.\n");
        return 0;
    }

    TiXmlElement* pLightElement = new TiXmlElement("light");
		pLightElement->SetAttribute("name", parent->Attribute("name"));
		pLightElement->SetAttribute("id", id_counter);
		pLightElement->SetAttribute("type", getLightTypeString(lightType).c_str());
		pLightElement->SetAttribute("castShadows", getBoolString(pGameLight->CastShadows()).c_str());
		parent->LinkEndChild(pLightElement);

    IGameProperty* pGameProperty = pGameLight->GetLightColor();
    float propertyValue;
    Point3 lightColor;
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(lightColor);
    }

		TiXmlElement* pColourElement = new TiXmlElement("colourDiffuse");
		pColourElement->SetDoubleAttribute("r", lightColor.x);
		pColourElement->SetDoubleAttribute("g", lightColor.y);
		pColourElement->SetDoubleAttribute("b", lightColor.z);
		pLightElement->LinkEndChild(pColourElement);
    
    TiXmlElement* pSpecColourElement = new TiXmlElement("colourSpecular");
		pSpecColourElement->SetDoubleAttribute("r", lightColor.x);
		pSpecColourElement->SetDoubleAttribute("g", lightColor.y);
		pSpecColourElement->SetDoubleAttribute("b", lightColor.z);
		pLightElement->LinkEndChild(pSpecColourElement);
		
		if(lightType == OGRE_LIGHT_POINT || lightType == OGRE_LIGHT_SPOT)
		{
      float attRange = 100.0f;
      float attConst = 1.0f;
      float attLinear = 0.0f;
      float attQuad = 0.0f;

      // Max lets attenuation start at a specific point.  Ogre seems to 
      // require that attenuation begins immediately.  We use the
      // Attenuation end as the range.
      pGameProperty = pGameLight->GetLightAttenEnd();
      if(pGameProperty)
      {
        pGameProperty->GetPropertyValue(attRange);
        if (attRange == 0.0f)
          attRange = 100.0f;
      }
      attRange = attRange * mParams.lum;

      // Inverse decay
      if(pGameLight->GetLightDecayType() == 1)
      {
        attLinear = 1.0f;
      }
      // Inverse Square decay
      else if(pGameLight->GetLightDecayType() == 2)
      {
        attQuad = 1.0f;
      }

			TiXmlElement* pAttenuationElement = new TiXmlElement("lightAttenuation");
			pAttenuationElement->SetDoubleAttribute("range", attRange);
			pAttenuationElement->SetDoubleAttribute("constant", attConst);
			pAttenuationElement->SetDoubleAttribute("linear", attLinear);
			pAttenuationElement->SetDoubleAttribute("quadratic", attQuad);
			pLightElement->LinkEndChild(pAttenuationElement);
		}

		if(lightType == OGRE_LIGHT_SPOT)
		{
      float rangeInner = 0.0f;
      float rangeFalloff  = 1.0f;
      float rangeOuter = 1.0f;
      pGameProperty = pGameLight->GetLightFallOff();
      if(pGameProperty)
      {
        // Max seems to use GetLightFallOff to get the outer angle.  
        // This doesn't leave anything for falloff. 
        pGameProperty->GetPropertyValue(propertyValue);	
        rangeOuter = propertyValue * PI / 180.0f;
      }
      pGameProperty = pGameLight->GetLightHotSpot();
      if(pGameProperty)
      {
        pGameProperty->GetPropertyValue(propertyValue);	
        rangeInner = propertyValue * PI / 180.0f;
      }

			TiXmlElement* pAttenuationElement = new TiXmlElement("lightRange");
			pAttenuationElement->SetDoubleAttribute("inner", rangeInner);
			pAttenuationElement->SetDoubleAttribute("outer", rangeOuter);
			pAttenuationElement->SetDoubleAttribute("falloff", rangeFalloff);
			pLightElement->LinkEndChild(pAttenuationElement);
		}

    id_counter++;
		return pLightElement;
  }

	bool ExScene::writeSceneFile()
	{    
		return xmlDoc->SaveFile(scenePath.c_str());
	}

}; //end of namespace
