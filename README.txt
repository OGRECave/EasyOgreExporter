/**********************************/
/*     EASY OGRE MAX EXPORTER     */
/**********************************/

/**********************************/
/* Author     : Bastien BOURINEAU */
/* Start Date : January 13, 2012  */
/* Licence    : LGPL V2 or later  */
/**********************************/
////////////////////////////////////


#Version 0.7 : Febrary 28, 2012

 - Ogre Scene file
 - meshs
 - materials converted from the ogre standard materials, with colors, textures, multi uv and transparency
 - biped skeletons
 - skeletons animation from the main track or from the biped mixer clips if available
 - Nodes animations included in the Ogre Scene file (from the main animation track)
 - Mesh Poses
 - Mesh Poses Animations
 - Mesh morph animations
 - animations from the motion mixer 
 - Manage units convertion from max to meter
 - Export interface with options
   - resources subdirs
   - material prefix
   - Ogre version
   - advanced configuration
   - automatic LOD generation

Available object user properties
This attributes can be added from the Object property dialog in Max
 - renderingDistance, this attribute is exporter in the ogre scene file, it define the visibility distance of a mesh 
  * ex : renderingDistance=10.5
 - noLOD, override the LOD generation for the specified object
  * ex : noLOD=false

#TODO
 - Manage Mental ray materials if possible
 - Manage Automatic Shader creation in materials from DX normal maps
   
 