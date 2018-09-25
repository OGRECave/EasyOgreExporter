/**********************************/
/*     EASY OGRE MAX EXPORTER     */
/**********************************/

/**********************************************************/
/* Author       : Bastien BOURINEAU                       */
/* Start Date   : January 13, 2012                        */
/* Licence      : LGPL V2 or later                        */
/* Contributors : Nir Benda (BJoe) for UNICODE conversion */
/**********************************************************/
////////////////////////////////////

Ogre wiki page : http://www.ogre3d.org/tikiwiki/Easy+Ogre+Exporter
Ogre forum thread : http://www.ogre3d.org/forums/viewtopic.php?f=8&t=68688

#Version 3.2.0 : September 25 2018

 - Ogre Scene file
 - meshs
 - materials :
  - converted from the max standard materials, with colors, textures, multi uv and transparency
  - converted from the Architecture materials, with colors, diffuse texture, multi uv and transparency
  - converted from the Mentalray Archi / Design materials, with colors, diffuse texture, multi uv and transparency
  - Automatic Shader creation for pixel lighting and normal / specular maps with a standard technique for non supported hardware
  - Convert texures to DDS format
 - biped skeletons
 - skeletons animation from the main track or from the biped mixer clips if available
 - Nodes animations included in the Ogre Scene file (from the main animation track)
 - Mesh Poses
 - Mesh Poses Animations
 - Mesh morph animations
 - animations from the motion mixer 
 - animations clips from track tags
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

Materials use :
 - disable "PreMultiplied Alpha" option on diffuse textures with an alpha channel to use alpha_rejection (usefull for folliages)

 - Setup (automatic install)
   

#Build
to build the exporter it's a little complicated because of some max sdk components :/

- create an ext directory in EOE project directory

Ogre 32 & 64b in static :
- rebuild all Ogre dependencies with _SECURE_SCL=0 in preprocessor
- rebuild ogre with _SECURE_SCL=0 in preprocessor

- copy the ogre 32 to ext/ogre3d_secure_scl/sdk/static (bin/, lib/, include/)
- copy the ogre 64 to ext/ogre3d_secure_scl/sdk_x64/static (bin/, lib/, include/)

Max SDK: (for each max sdk ^^)
- build the maxsdk\samples\modifiers\morpher lib in 32 & 64b
- copy max sdk in ext/maxversion ex: ext/2012/include, ext/2012/lib, ext/2012/libx64
- also copy the morpher sample lib and include in the corresponding directories

Then you can open the EOE vcproj and build.

To debug in max you need to build in hybrid mode and launch max from the debugger