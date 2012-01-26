////////////////////////////////////////////////////////////////////////////////
// scene.cpp
// Author     : Doug Perkowski
// Start Date : March 27th, 2008
// Copyright  : Copyright (c) 2002-2011 OC3 Entertainment, Inc.
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
#include "scene.h"
#include "EasyOgreExporterLog.h"

namespace EasyOgreExporter
{
	/***** Class EasyOgreScene *****/
	// constructor
	EasyOgreScene::EasyOgreScene()
	{
		id_counter = 0;
	}

	// destructor
	EasyOgreScene::~EasyOgreScene()
	{
	}
	void EasyOgreScene::clear()
	{
		m_lights.clear();
		m_meshes.clear();
		m_cameras.clear();
		id_counter = 0;
	}
	void EasyOgreScene::addLight(EasyOgreLight& light)
	{
		id_counter++;
		light.node.id = id_counter;
		m_lights.push_back(light);
	}
	void EasyOgreScene::addCamera(EasyOgreCamera& camera)
	{
		id_counter++;
		camera.node.id = id_counter;
		m_cameras.push_back(camera);
	}
	void EasyOgreScene::addMesh(EasyOgreMesh& mesh)
	{
		id_counter++;
		mesh.node.id = id_counter;
		m_meshes.push_back(mesh);
	}

	std::string EasyOgreScene::getLightTypeString(EasyOgreLightType type)
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

	TiXmlElement* EasyOgreScene::writeNodeData(TiXmlElement* parent, const EasyOgreNode &node)
	{
		if(!parent)
		{
			return NULL;
		}
		TiXmlElement *pNodeElement = new TiXmlElement("node");
		pNodeElement->SetAttribute("name", node.name.c_str());
		pNodeElement->SetAttribute("id", node.id);
		pNodeElement->SetAttribute("isTarget", "false");
		parent->LinkEndChild(pNodeElement);

		TiXmlElement *pPositionElement = new TiXmlElement("position");
		pPositionElement->SetDoubleAttribute("x", node.trans.pos.x);
		pPositionElement->SetDoubleAttribute("y", node.trans.pos.y);
		pPositionElement->SetDoubleAttribute("z", node.trans.pos.z);
		pNodeElement->LinkEndChild(pPositionElement);

		TiXmlElement *pRotationElement = new TiXmlElement("rotation");
		pRotationElement->SetDoubleAttribute("qx", node.trans.rot.x);
		pRotationElement->SetDoubleAttribute("qy", node.trans.rot.y);
		pRotationElement->SetDoubleAttribute("qz", node.trans.rot.z);
		// Notice that in Max we flip the w-component of the quaternion;
		pRotationElement->SetDoubleAttribute("qw", -node.trans.rot.w);
		pNodeElement->LinkEndChild(pRotationElement);

		TiXmlElement *pScaleElement = new TiXmlElement("scale");
		pScaleElement->SetDoubleAttribute("x", node.trans.scale.x);
		pScaleElement->SetDoubleAttribute("y", node.trans.scale.y);
		pScaleElement->SetDoubleAttribute("z", node.trans.scale.z);
		pNodeElement->LinkEndChild(pScaleElement);
		return pNodeElement;
	}

	bool EasyOgreScene::writeSceneFile(ParamList &params)
	{
		TiXmlDocument xmlDoc;
		TiXmlElement *pSceneElement = new TiXmlElement("scene");
		xmlDoc.LinkEndChild(pSceneElement);

		pSceneElement->SetAttribute("formatVersion", "1.0");
		pSceneElement->SetAttribute("minOgreVersion", "1.4");
		pSceneElement->SetAttribute("author", "EasyOgreExporter");

		TiXmlElement *pNodesElement = new TiXmlElement("nodes");
		pSceneElement->LinkEndChild(pNodesElement);

		// Warning! Exporting with the Y-Axis up won't work in FaceFX Studio because the bone 
		// transforms will not be in Max native coords, but in the FaceFX file they are.
		if(!params.yUpAxis)
		{
			// FaceFX FXA exporters export in the coordinate system that is native
			// to the animation package they were created in.  When displayed in
			// OGRE, everything needs to be converted to Y-up.  This is done by 
			// rotating the entire scene by thge appropriate amount.  In Max, the 
			// appropriate amount is a -90 degree rotation around the X axis.
			TiXmlComment * comment = new TiXmlComment();
			comment->SetValue(" Rotation exists so FaceFX .FXA exporters can use Native Max coordinates. ");  
			pNodesElement->LinkEndChild(comment);
			TiXmlElement *pRotationElement = new TiXmlElement("rotation");
			pRotationElement->SetAttribute("axisX", "1");
			pRotationElement->SetAttribute("axisY", "0");
			pRotationElement->SetAttribute("axisZ", "0");
			pRotationElement->SetAttribute("angle", "-90");
			pNodesElement->LinkEndChild(pRotationElement);
		}
		
		for(int i = 0; i < m_meshes.size(); ++i)
		{
			TiXmlElement *pNodeElement = writeNodeData(pNodesElement, m_meshes[i].node);
			TiXmlElement *pEntityElement = new TiXmlElement("entity");
			pEntityElement->SetAttribute("name", m_meshes[i].node.name.c_str());
			pEntityElement->SetAttribute("id", m_meshes[i].node.id);
			pEntityElement->SetAttribute("meshFile", m_meshes[i].meshFile.c_str());
			pEntityElement->SetAttribute("castShadows", "true");
			pEntityElement->SetAttribute("receiveShadows", "true");
			pNodeElement->LinkEndChild(pEntityElement);
		}

		// A light to compare to to see if we've modified anything from default values.
		EasyOgreLight compObj;
		for(int i = 0; i < m_lights.size(); ++i)
		{
			TiXmlElement *pNodeElement = writeNodeData(pNodesElement, m_lights[i].node);
			if(pNodeElement)
			{
				TiXmlElement *pLightElement = new TiXmlElement("light");
				pLightElement->SetAttribute("name", m_lights[i].node.name.c_str());
				pLightElement->SetAttribute("id", m_lights[i].node.id);
				pLightElement->SetAttribute("type", getLightTypeString(m_lights[i].type).c_str());
				pLightElement->SetAttribute("castShadows", "false");
				pNodeElement->LinkEndChild(pLightElement);

				TiXmlElement *pNormalElement = new TiXmlElement("normal");
				pNormalElement->SetDoubleAttribute("x", 0.0f);
				pNormalElement->SetDoubleAttribute("y", 0.0f);
				pNormalElement->SetDoubleAttribute("z", -1.0f);
				pLightElement->LinkEndChild(pNormalElement);

				if(	fabs(m_lights[i].diffuseColour.x - compObj.diffuseColour.x) > PRECISION	||
					fabs(m_lights[i].diffuseColour.y - compObj.diffuseColour.y) > PRECISION	||
					fabs(m_lights[i].diffuseColour.z - compObj.diffuseColour.z) > PRECISION		)
				{
					TiXmlElement *pColourElement = new TiXmlElement("colourDiffuse");
					pColourElement->SetDoubleAttribute("r", m_lights[i].diffuseColour.x);
					pColourElement->SetDoubleAttribute("g", m_lights[i].diffuseColour.y);
					pColourElement->SetDoubleAttribute("b", m_lights[i].diffuseColour.z);
					pLightElement->LinkEndChild(pColourElement);
				}

				if(	fabs(m_lights[i].specularColour.x - compObj.specularColour.x) > PRECISION	||
					fabs(m_lights[i].specularColour.y - compObj.specularColour.y) > PRECISION	||
					fabs(m_lights[i].specularColour.z - compObj.specularColour.z) > PRECISION		)
				{
					TiXmlElement *pSpecColourElement = new TiXmlElement("colourSpecular");
					pSpecColourElement->SetDoubleAttribute("r", m_lights[i].specularColour.x);
					pSpecColourElement->SetDoubleAttribute("g", m_lights[i].specularColour.y);
					pSpecColourElement->SetDoubleAttribute("b", m_lights[i].specularColour.z);
					pLightElement->LinkEndChild(pSpecColourElement);
				}
				if(	m_lights[i].type == OGRE_LIGHT_POINT)
				{
					TiXmlElement *pAttenuationElement = new TiXmlElement("lightAttenuation");
					pAttenuationElement->SetDoubleAttribute("range", m_lights[i].attenuation_range);
					pAttenuationElement->SetDoubleAttribute("constant", m_lights[i].attenuation_constant);
					pAttenuationElement->SetDoubleAttribute("linear", m_lights[i].attenuation_linear);
					pAttenuationElement->SetDoubleAttribute("quadratic", m_lights[i].attenuation_quadratic);
					pLightElement->LinkEndChild(pAttenuationElement);
				}
				if(	m_lights[i].type == OGRE_LIGHT_SPOT	)
				{
					TiXmlElement *pAttenuationElement = new TiXmlElement("lightRange");
					pAttenuationElement->SetDoubleAttribute("inner", m_lights[i].range_inner);
					pAttenuationElement->SetDoubleAttribute("outer", m_lights[i].range_outer);
					pAttenuationElement->SetDoubleAttribute("falloff", m_lights[i].range_falloff);
					pLightElement->LinkEndChild(pAttenuationElement);
				}
			}
		}
		for(int i = 0; i < m_cameras.size(); ++i)
		{
			TiXmlElement *pNodeElement = writeNodeData(pNodesElement, m_cameras[i].node);
			if(pNodeElement)
			{
				TiXmlElement *pCameraElement = new TiXmlElement("camera");
				pCameraElement->SetAttribute("name", m_cameras[i].node.name.c_str());
				pCameraElement->SetAttribute("id", m_cameras[i].node.id);
				pCameraElement->SetDoubleAttribute("fov", m_cameras[i].fov);
				pNodeElement->LinkEndChild(pCameraElement);

				TiXmlElement *pClippingElement = new TiXmlElement("clipping");
				pClippingElement->SetDoubleAttribute("near", m_cameras[i].clipNear);
				pClippingElement->SetDoubleAttribute("far", m_cameras[i].clipFar);
				pCameraElement->LinkEndChild(pClippingElement);
			}
		}
    
    std::string filePath = params.outputDir + "\\" + params.sceneFilename + ".scene";
		xmlDoc.SaveFile(filePath.c_str());
		return true;
	}

}; //end of namespace
