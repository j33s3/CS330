///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials() {

	//create the material for the base of the shaker
	OBJECT_MATERIAL designMaterial;
	designMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	designMaterial.ambientStrength = 0.01f;
	designMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	designMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.35f);
	designMaterial.shininess = 20.0;
	designMaterial.tag = "design";

	m_objectMaterials.push_back(designMaterial);

	//create the material for the top of the shaker
	OBJECT_MATERIAL brownMaterial;
	brownMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	brownMaterial.ambientStrength = 0.01f;
	brownMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	brownMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	brownMaterial.shininess = 20.0f;
	brownMaterial.tag = "brown";

	m_objectMaterials.push_back(brownMaterial);

	//create the material for the counter top (plane)
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	tableMaterial.ambientStrength = 0.01f;
	tableMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	tableMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	tableMaterial.shininess = 30.0f;
	tableMaterial.tag = "table";

	m_objectMaterials.push_back(tableMaterial);

	OBJECT_MATERIAL napkinMaterial;
	napkinMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	napkinMaterial.ambientStrength = 0.005f;
	napkinMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	napkinMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	napkinMaterial.shininess = 0.5f;
	napkinMaterial.tag = "napkin";

	m_objectMaterials.push_back(napkinMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene. There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights() {

	//light from the kitchen
	m_pShaderManager->setVec3Value("lightSources[0].position", -7.0f, 8.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.5f, 0.5f, 0.45f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.1f, 0.1f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.9f, 0.9f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.9f);

	//trying to mimic light from window
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 7.0f, 15.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.5f, 0.5f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 7.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

}


 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures() {

	//Creating the textures
	CreateGLTexture("../../Utilities/textures/customTexture.jpg", "customTexture");
	CreateGLTexture("../../Utilities/textures/customTexture2.jpg", "customTexture2");
	CreateGLTexture("../../Utilities/textures/table_wood.jpg", "table_wood");
	CreateGLTexture("../../Utilities/textures/butter_tray.jpg", "butter_tray");
	CreateGLTexture("../../Utilities/textures/napkin_holder.jpg", "napkin_holder");
	BindGLTextures();
}



/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();

	// add and define the light sources for the scene
	SetupSceneLights();


	m_basicMeshes->LoadPlaneMesh();

	m_basicMeshes->LoadCylinderMesh();

	m_basicMeshes->LoadSphereMesh();

	m_basicMeshes->LoadTorusMesh();

	m_basicMeshes->LoadBoxMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/


	/****************************************************************/
	//**				  Drawing the Table Plane				  **//
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 25.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set the color of the shader (brown)
	//SetShaderColor(0.130, 0.086, 0.046, 1);
	SetShaderColor(0.1f, 0.084f, 0.052f, 1.0f);
	//set the material for the shader
	SetShaderMaterial("table");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	//**				  Drawing Salt Shaker					  **//
	/****************************************************************/

	// set the rotation on the X and Y axis
	XrotationDegrees = 0.0f;
	YrotationDegrees = 55.0f;

	//set the scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 2.5f, 1.4f);

	// set the position for the mesh
	positionXYZ = glm::vec3(4.2f, 1.2f, 2.8f);

	//set the tansformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set the color for the shader (light blue)
	//SetShaderColor(0.624, 0.812, 0.936, 1);

	//setting the scale for the texture and loading it
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderTexture("customTexture");
	//set the material of the shader
	SetShaderMaterial("design");


	//draw the mesh with transformations
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//set the scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);

	//set the position for the mesh
	positionXYZ = glm::vec3(4.2f, 3.7f, 2.8f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//change the color of the shader (brown)
	//SetShaderColor(0.244, 0.192, 0.208, 1);

	//setting the scale for the texture then loading it
	SetTextureUVScale(4.0f, 3.0f);
	SetShaderTexture("customTexture2");
	//set the material or the shader
	SetShaderMaterial("brown");

	//draw the mesh with transformations
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	//**				  Drawing Pepper Shaker					  **//
	/****************************************************************/

	//set the rotation on the Y axis
	YrotationDegrees = 85.0f;

	//set the scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 2.5f, 1.4f);

	// set the position for the mesh
	positionXYZ = glm::vec3(-3.5f, 1.2f, 2.5f);

	//set the tansformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set the color for the shader (light blue)
	//SetShaderColor(0.624, 0.812, 0.936, 1);

	//setting the scale for the texture and loading it
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderTexture("customTexture");

	//set the material of the shader
	SetShaderMaterial("design");


	//draw the mesh with transformations
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//set the scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);

	//set the position for the mesh
	positionXYZ = glm::vec3(-3.5f, 3.7f, 2.5f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//change the color of the shader (brown)
	//SetShaderColor(0.244, 0.192, 0.208, 1);

	//setting the scale for the texture then loading it
	SetTextureUVScale(4.0f, 3.0f);
	SetShaderTexture("customTexture2");

	//set the material or the shader
	SetShaderMaterial("brown");

	//draw the mesh with transformations
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	//**				  Drawing the Table Tray				  **//
	/****************************************************************/
	
	// reset the rotation on the Y axis
	YrotationDegrees = 0.0f;

	//set the scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.9f, 7.0f);

	//set the position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.3f, 0.0f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set the texture for the mesh
	SetShaderTexture("table_wood");

	// set the material for the mesh
	SetShaderMaterial("table");

	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//set the rotation on the X axis
	XrotationDegrees = 90;

	//set the scale for the mesh
	scaleXYZ = glm::vec3(6.82f, 6.82f, 6.82f);

	//set the position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.1f, 0.0f);

	//set the tansformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// reload the mesh with thickness of 0.03
	m_basicMeshes->LoadTorusMesh(0.03);

	// draw the mesh
	m_basicMeshes->DrawTorusMesh();


	/****************************************************************/
	//**					  Drawing Butter Dish				  **//
	/****************************************************************/
	
	
	//reset the roation on X & Y axis
	XrotationDegrees = 0;
	YrotationDegrees = 140.0f;

	//set the scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.0f, 3.0f);

	//set the position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.3f, 3.3f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the scale for the texture
	SetTextureUVScale(2.0, 2.0);

	//set the texture for the shader
	SetShaderTexture("butter_tray");

	//set the material for the shader
	SetShaderMaterial("design");

	// draw the mesh
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/

	//set the rotation on the X axis
	XrotationDegrees = 90;
	YrotationDegrees = 0;
	ZrotationDegrees = 40;

	//set the scale for the mesh
	scaleXYZ = glm::vec3(1.7f, 3.2f, 3.5f);

	//set the position for the mesh
	//positionXYZ = glm::vec3(0.0f, 2.0f, 6.0f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set the scale for the texture
	SetTextureUVScale(4.0f, 3.0);

	//set the texture for the shader
	SetShaderTexture("customTexture2");

	//set the material for the shader
	SetShaderMaterial("brown");
	// reload the mesh with thickness 0.11
	m_basicMeshes->LoadTorusMesh(0.11f);
	// draw the mesh
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	//**				  Drawing the Napkin Holder				  **//
	/****************************************************************/

	/******************************************/
	//**	Starting with Disign Segments	**//
	/******************************************/

	// Reset the rotation on x and z axis
	XrotationDegrees = 0;
	YrotationDegrees = 20;
	ZrotationDegrees = 0;

	// set the scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 5.0f, 0.5f);

	// set the position for the mesh
	positionXYZ = glm::vec3(-1.7279404685f, 3.4f, -2.0f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the scale for the texture
	SetTextureUVScale(1.0f, 1.0f);

	// set the texture for the shader
	SetShaderTexture("napkin_holder");

	//set the material for the shader
	SetShaderMaterial("design");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//****************************************************************/

	// set the position for the mesh
	positionXYZ = glm::vec3(-1.0f, 3.4f, 0.0f);

	//set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//****************************************************************/

	// Set the scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 0.5f, 2.0f);

	// set the position for the mesh
	positionXYZ = glm::vec3(-1.3639702343, 1.15f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// reset the texture
	SetShaderTexture("");
	// set the color for the shader
	SetShaderColor(0.596f, 0.708f, 0.780f, 1);

	// Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/******************************************/
	//**	Starting with napkins Segments	**//
	/******************************************/
	// Set the rotation on the X and Z axis
	XrotationDegrees = 90;
	YrotationDegrees = 0;
	ZrotationDegrees = -20;

	// set the scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 3.0f, 2.5f);

	//set the color for the shader
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);

	//set the material for the shader
	SetShaderMaterial("napkin");

	for (int i = 1; i <= 10; i++) {

		// Calculate the X and Z positions
		double Zpos = i / 6.0;
		double Xpos = -1.0 + (tan(ZrotationDegrees * 3.14159 / 180) * Zpos);

		// set the position for the mesh
		positionXYZ = glm::vec3(Xpos, 4.0f, -Zpos);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// draw the mesh
		m_basicMeshes->DrawPlaneMesh();
	}

	///****************************************************************/
}
