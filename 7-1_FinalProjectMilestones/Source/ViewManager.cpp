///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	//g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	//g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	//g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	//g_pCamera->Zoom = 80;

	// Custom Camera View Parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 10.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.2f, -0.5f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;

	// Defaults for movement speed and sensitivity
	g_pCamera->MovementSpeed = 1.0f;
	g_pCamera->MouseSensitivity = 0.01f;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// this callback is used to recieve scroll wheel events
	glfwSetScrollCallback(window, &ViewManager::Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	//When first mouse event occurs this must be recorded
	//calculates the x and y position offsets
	if (gFirstMouse) {
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	//calculate the X and Y offsets
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos;

	// set the positions as the last position
	gLastX = xMousePos;
	gLastY = yMousePos;

	//move the camera to the offsets accordingly
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Scroll_Callback()
 *
 *  This method is automatically called from GLFW whenever.
 ***********************************************************/
void ViewManager::Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset) {


	//I had to make changes to this function
	//Instead of updating the value directly, I needed to make copies to compute value comparisons.
	//I had issues with the system letting negative values pass through, this only occured when user scrolled quickly downward.

	
	// will take update the movement speed and sensitivity, if the value is less than zero than the update is ignored
	// making copies of the values
	double movement = g_pCamera->MovementSpeed;
	double sens = g_pCamera->MouseSensitivity;

	//updating values
	movement += yOffset;
	sens += yOffset / 100;

	// checking validity
	if (movement < 0 || sens < 0) {
		movement = 0;
		sens = 0;
	}

	// making changes
	g_pCamera->MovementSpeed = movement;
	g_pCamera->MouseSensitivity = sens;

}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zoom
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS) {
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS) {
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS) {
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS) {
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// proccess camera veritcal
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS) {
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS) {
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}

	// proccess viewing mode
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS) {
		bOrthographicProjection = false;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS) {
		bOrthographicProjection = true;
	}
}


/***********************************************************
 *  UpdateProjectionMatrix
 *
 *  This method is called for updating the projection matrix
 *  'p' for perspective 'o' for orthogonal
 ***********************************************************/
void ViewManager::UpdateProjectionMatrix() {
	glm::mat4 projection;

	//If the view is Ortho
	if (bOrthographicProjection) {
		projection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, -250.0f, 250.0f);
	}
	// else set it to projeciton
	else  {
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	//if the shaderManager is valid
	if (m_pShaderManager) {
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// update the current projection matrix
	UpdateProjectionMatrix();

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}