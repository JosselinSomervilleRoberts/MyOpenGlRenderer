// ----------------------------------------------
// Polytechnique - INF584 "Image Synthesis"
//
// Base code for practical assignments.
//
// Copyright (C) 2022 Tamy Boubekeur
// All rights reserved.
// ----------------------------------------------

#define _USE_MATH_DEFINES

#include <glad/glad.h>

#include <cstdlib>
#include <cstdio>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <exception>
#include <filesystem>

namespace fs = std::filesystem;

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Resources.h"
#include "Error.h"
#include "Console.h"
#include "MeshLoader.h"
#include "Scene.h"
#include "Image.h"
#include "Rasterizer.h"
#include "RayTracer.h"

using namespace std;

// Window parameters
static GLFWwindow * windowPtr = nullptr;
static std::shared_ptr<Scene> scenePtr;
static std::shared_ptr<Rasterizer> rasterizerPtr;
static std::shared_ptr<RayTracer> rayTracerPtr;

// Camera control variables
static glm::vec3 center = glm::vec3 (0.0); // To update based on the mesh position
static float meshScale = 1.0; // To update based on the mesh size, so that navigation runs at scale
static bool isRotating (false);
static bool isPanning (false);
static bool isZooming (false);
static double baseX (0.0), baseY (0.0);
static glm::vec3 baseTrans (0.0);
static glm::vec3 baseRot (0.0);

// Files
static std::string basePath;
static std::string meshFilename;

// Raytraced rendering
static bool isDisplayRaytracing (false);

// Diagnostic
static int diagnostic = 1;

void clear ();

std::string boolTotext(bool b) {
	if (b) return "On ";
	else return "Off";
}

void printHelp () {
	Console::print (std::string ("Help:\n") 
			  + "\tMouse commands:\n" 
			  + "\t* Left button: rotate camera\n" 
			  + "\t* Middle button: zoom\n" 
			  + "\t* Right button: pan camera\n" 
			  + "\tKeyboard commands:\n" 
   			  + "\t* ESC: quit the program\n"
   			  + "\t* H: print this help\n"
   			  + "\t* F12: reload GPU shaders\n"
   			  + "\t* F: decrease field of view\n"
   			  + "\t* G: increase field of view\n"
   			  + "\t* TAB: switch between rasterization and ray tracing display\n"
   			  + "\t* SPACE: execute ray tracing\n"
		      + "\t* A: enable/disable acceleration ray tracing with BVH\n"
		      + "\t* O: enable/disable occlusion in ray tracing\n"
		      + "\t* P: enable/disable anti-aliasing in ray tracing\n"
		      + "\n"
		      + "\n Diagnostic and SSR:\n"
		      + "\t* F1: render (SSR: also reset booleans togglers) \n"
		      + "\t* F2: normal\n"
		      + "\t* F3: albedo\n"
		      + "\t* F4: tex coordinates\n"
		      + "\t* F5: (SSR) reflected\n"
		      + "\t* F6: (SSR) " + boolTotext(rasterizerPtr->useReflectedShading) + ": reflected shading\n"
		      + "\t* F7: (SSR) " + boolTotext(rasterizerPtr->useBinary) + ": binary search\n"
		      + "\t* F8: (SSR) " + boolTotext(rasterizerPtr->useAntiAlias) + ": Anti-alias\n"
		      + "\t* F9: (SSR) " + boolTotext(rasterizerPtr->useInTexture) + ": march in texture\n"
		      + "\t*F10: (SSR) " + boolTotext(rasterizerPtr->allowBehindCamera) + ": allow reflections behind camera\n"
		      + "\t*F11: (SSR) " + boolTotext(rasterizerPtr->useScreenEdge) + ": (Shading) use screen edge factor\n"
		      + "\t*  D: (SSR) " + boolTotext(rasterizerPtr->useDirectionShading) + ": (Shading) use reflection direction\n"
		      + "\t*  R: (SSR) Linear Steps : " + std::to_string(rasterizerPtr->SSR_linear_steps) + "\n"
		      + "\t*  T: (SSR) Ray thickness: " + std::to_string(rasterizerPtr->SSR_thickness) + "\n");
}

/// Adjust the ray tracer target resolution and runs it.
void raytrace () {
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);
	rayTracerPtr->setResolution (width, height);
	rayTracerPtr->render (scenePtr);
}

/// Executed each time a key is entered.
void keyCallback (GLFWwindow * windowPtr, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_H) {
			printHelp ();
		} else if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose (windowPtr, true); // Closes the application if the escape key is pressed
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F12) {
			rasterizerPtr->loadShaderProgram (basePath);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F) {
			scenePtr->camera()->setFoV (std::max (5.f, scenePtr->camera()->getFoV () - 5.f));
		} else if (action == GLFW_PRESS && key == GLFW_KEY_Q) { // A on a french keyboard
			rayTracerPtr->useBVH =!(rayTracerPtr->useBVH);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_O) { // O on a french keyboard
			rayTracerPtr->useOcclusion =!(rayTracerPtr->useOcclusion);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_P) { // P on a french keyboard
			if (rayTracerPtr->alias_number > 1) rayTracerPtr->alias_number = 1;
			else rayTracerPtr->alias_number = 3;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_G) {
			scenePtr->camera()->setFoV (std::min (120.f, scenePtr->camera()->getFoV () + 5.f));
		} else if (action == GLFW_PRESS && key == GLFW_KEY_TAB) {
			isDisplayRaytracing = !isDisplayRaytracing;
			//if(isDisplayRaytracing) rayTracerPtr->render (scenePtr);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_SPACE) {
			raytrace ();
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F1) {
			diagnostic = 1;
			rasterizerPtr->useReflectedShading = true;
			rasterizerPtr->useBinary = true;
			rasterizerPtr->useAntiAlias = false;
			rasterizerPtr->useInTexture = true;
			rasterizerPtr->allowBehindCamera = false;
			rasterizerPtr->useScreenEdge = true;
			rasterizerPtr->useDirectionShading = true;
			rasterizerPtr->SSR_linear_steps = 500;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F2) {
			diagnostic = 2;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F3) {
			diagnostic = 3;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F4) {
			diagnostic = 4;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F5) {
			diagnostic = 5;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F6) {
			rasterizerPtr->useReflectedShading = !(rasterizerPtr->useReflectedShading);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F7) {
			rasterizerPtr->useBinary = !(rasterizerPtr->useBinary);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F8) {
			rasterizerPtr->useAntiAlias = !(rasterizerPtr->useAntiAlias);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F9) {
			rasterizerPtr->useInTexture = !(rasterizerPtr->useInTexture);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F10) {
			rasterizerPtr->allowBehindCamera = !(rasterizerPtr->allowBehindCamera);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F11) {
			rasterizerPtr->useScreenEdge = !(rasterizerPtr->useScreenEdge);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_D) {
			rasterizerPtr->useDirectionShading = !(rasterizerPtr->useDirectionShading);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_R) {
			if(rasterizerPtr->SSR_linear_steps == 25) rasterizerPtr->SSR_linear_steps = 50;
			else if(rasterizerPtr->SSR_linear_steps == 50) rasterizerPtr->SSR_linear_steps = 100;
			else if(rasterizerPtr->SSR_linear_steps == 100) rasterizerPtr->SSR_linear_steps = 250;
			else if(rasterizerPtr->SSR_linear_steps == 250) rasterizerPtr->SSR_linear_steps = 500;
			else if(rasterizerPtr->SSR_linear_steps == 500) rasterizerPtr->SSR_linear_steps = 1000;
			else if(rasterizerPtr->SSR_linear_steps == 1000) rasterizerPtr->SSR_linear_steps = 2500;
			else if(rasterizerPtr->SSR_linear_steps == 2500) rasterizerPtr->SSR_linear_steps = 25;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_T) {
			float EPSILON = 0.001f;
			if(rasterizerPtr->SSR_thickness <= 0.01 + EPSILON) rasterizerPtr->SSR_thickness = 0.025;
			else if(rasterizerPtr->SSR_thickness <= 0.025 + EPSILON) rasterizerPtr->SSR_thickness = 0.05;
			else if(rasterizerPtr->SSR_thickness <= 0.05 + EPSILON) rasterizerPtr->SSR_thickness = 0.1;
			else if(rasterizerPtr->SSR_thickness <= 0.1 + EPSILON) rasterizerPtr->SSR_thickness = 0.25;
			else if(rasterizerPtr->SSR_thickness <= 0.25 + EPSILON) rasterizerPtr->SSR_thickness = 0.5;
			else if(rasterizerPtr->SSR_thickness <= 0.5 + EPSILON) rasterizerPtr->SSR_thickness = 1;
			else if(rasterizerPtr->SSR_thickness <= 1 + EPSILON) rasterizerPtr->SSR_thickness = 0.01;
		} else {
			printHelp ();
		}
	}
}

/// Called each time the mouse cursor moves
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	int width, height;
	glfwGetWindowSize (windowPtr, &width, &height);
	float normalizer = static_cast<float> ((width + height)/2);
	float dx = static_cast<float> ((baseX - xpos) / normalizer);
	float dy = static_cast<float> ((ypos - baseY) / normalizer);
	if (isRotating) {
		glm::vec3 dRot (-dy * M_PI, dx * M_PI, 0.0);
		scenePtr->camera()->setRotation (baseRot + dRot);
	} else if (isPanning) {
		scenePtr->camera()->setTranslation (baseTrans + meshScale * glm::vec3 (dx, dy, 0.0));
	} else if (isZooming) {
		scenePtr->camera()->setTranslation (baseTrans + meshScale * glm::vec3 (0.0, 0.0, dy));
	}
}

/// Called each time a mouse button is pressed
void mouseButtonCallback (GLFWwindow * window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    	if (!isRotating) {
    		isRotating = true;
    		glfwGetCursorPos (window, &baseX, &baseY);
    		baseRot = scenePtr->camera()->getRotation ();
        } 
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    	isRotating = false;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    	if (!isPanning) {
    		isPanning = true;
    		glfwGetCursorPos (window, &baseX, &baseY);
    		baseTrans = scenePtr->camera()->getTranslation ();
        } 
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    	isPanning = false;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
    	if (!isZooming) {
    		isZooming = true;
    		glfwGetCursorPos (window, &baseX, &baseY);
    		baseTrans = scenePtr->camera()->getTranslation ();
        } 
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
    	isZooming = false;
    }
}

/// Executed each time the window is resized. Adjust the aspect ratio and the rendering viewport to the current window. 
void windowSizeCallback (GLFWwindow * windowPtr, int width, int height) {
	scenePtr->camera()->setAspectRatio (static_cast<float>(width) / static_cast<float>(height));
	rasterizerPtr->setResolution (width, height);
	rayTracerPtr->setResolution (width, height);
}

void initGLFW () {
	// Initialize GLFW, the library responsible for window management
	if (!glfwInit ()) {
		Console::print ("ERROR: Failed to init GLFW");
		std::exit (EXIT_FAILURE);
	}

	// Before creating the window, set some option flags
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint (GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint (GLFW_SAMPLES, 16); // Enable OpenGL multisampling (MSAA)

	// Create the window
	windowPtr = glfwCreateWindow (1024, 768, BASE_WINDOW_TITLE.c_str (), nullptr, nullptr);
	if (!windowPtr) {
		Console::print ("ERROR: Failed to open window");
		glfwTerminate ();
		std::exit (EXIT_FAILURE);
	}

	// Load the OpenGL context in the GLFW window using GLAD OpenGL wrangler
	glfwMakeContextCurrent (windowPtr);

	// Connect the callbacks for interactive control 
	glfwSetWindowSizeCallback (windowPtr, windowSizeCallback);
	glfwSetKeyCallback (windowPtr, keyCallback);
	glfwSetCursorPosCallback (windowPtr, cursorPosCallback);
	glfwSetMouseButtonCallback (windowPtr, mouseButtonCallback);
}

void initScene () {
	scenePtr = std::make_shared<Scene> ();
	scenePtr->setBackgroundColor (glm::vec3 (0.1f, 0.5f, 0.95f));

	// Mesh
	auto meshPtr = std::make_shared<Mesh> ();
	try {
		MeshLoader::loadOFF (meshFilename, meshPtr);
	} catch (std::exception & e) {
		exitOnCriticalError (std::string ("[Error loading mesh]") + e.what ());
	}
	meshPtr->computeBoundingSphere (center, meshScale);
	meshPtr->computePlanarParameterization();
	BoundingBox bbox = meshPtr->computeBoundingBox ();
	float extent = 2 * bbox.size ();
	if (meshFilename.find("sphere") != std::string::npos)
		meshPtr->setTranslation(glm::vec3(0, -0.2f * extent, 0));
    auto meshMaterialPtr = std::make_shared<Material> (glm::vec3 (0.05, 0.05, 0.05), 0.3, 0.2);
    scenePtr->add (meshPtr);
    scenePtr->addMaterial (meshMaterialPtr);
	scenePtr->setMaterialToMesh (0, 0);
	std::cout << meshFilename << std::endl;

	
	// Adding a ground adapted to the loaded model
	std::shared_ptr<Mesh> groundMeshPtr = std::make_shared<Mesh> ();
	std::cout << "EXTENT: " << extent << std::endl;
	glm::vec3 startP = bbox.center () + glm::vec3 (-extent, -bbox.height()/2.f, -extent);
	groundMeshPtr->vertexPositions().push_back (startP); 
	groundMeshPtr->vertexPositions().push_back (startP + glm::vec3 (0.f, 0.f, 2.f*extent));
	groundMeshPtr->vertexPositions().push_back (startP + glm::vec3 (2.f*extent, 0.f, 2.f*extent));
	groundMeshPtr->vertexPositions().push_back (startP + glm::vec3 (2.f*extent, 0.f, 0.f));
	groundMeshPtr->triangleIndices().push_back (glm::uvec3 (0, 1, 2));
	groundMeshPtr->triangleIndices().push_back (glm::uvec3 (0, 2, 3));
	groundMeshPtr->recomputePerVertexNormals ();
    auto groundMaterialPtr = std::make_shared<Material> (glm::vec3 (0.6, 0.6, 0.6f), 0.1f, 0.9);
    scenePtr->add (groundMeshPtr);
    scenePtr->addMaterial (groundMaterialPtr);
	scenePtr->setMaterialToMesh (1, 1);


	// Adding a wall adapted to the loaded model
	std::shared_ptr<Mesh> wallMeshPtr = std::make_shared<Mesh> ();
	startP = bbox.center () + glm::vec3 (-extent, -bbox.height()/2.f, -extent);
	wallMeshPtr->vertexPositions().push_back (startP); 
	wallMeshPtr->vertexPositions().push_back (startP + glm::vec3 (2.f*extent, 0.f, 0.f));
	wallMeshPtr->vertexPositions().push_back (startP + glm::vec3 (2.f*extent, 2.f*extent, 0.f));
	wallMeshPtr->vertexPositions().push_back (startP + glm::vec3 (0.f, 2.f*extent, 0.f));
	wallMeshPtr->triangleIndices().push_back (glm::uvec3 (0, 1, 2));
	wallMeshPtr->triangleIndices().push_back (glm::uvec3 (0, 2, 3));
	wallMeshPtr->recomputePerVertexNormals ();
    auto wallMaterialPtr = std::make_shared<Material> (glm::vec3 (0.9, 0.5, 0.3f), 0.1f, 0.5f);
    scenePtr->add (wallMeshPtr);
    scenePtr->addMaterial (wallMaterialPtr);
	scenePtr->setMaterialToMesh (2, 2);

	// Light Sources
	//auto lightPtr = std::make_shared<LightSourceDir> ();
	//scenePtr->addLightSource(lightPtr);
	//auto lightPtr2 = std::make_shared<LightSourcePoint> ();
	//scenePtr->addLightSource(lightPtr2);
	float factor = 4;
	float distance = 1;
	scenePtr->addLightSource (std::make_shared<LightSourceDir> (distance * normalize (glm::vec3(0.f, -1.f, -1.f)), glm::vec3(1.f, 1.f, 1.f), factor*0.4f));
	scenePtr->addLightSource (std::make_shared<LightSourceDir> (distance * normalize (glm::vec3(-2.f, -0.5f, 0.f)), glm::vec3(0.2f, 0.6f, 1.f), factor*0.25f));
	scenePtr->addLightSource (std::make_shared<LightSourceDir> (distance * normalize (glm::vec3(2.f, -0.5f, 0.f)), glm::vec3(1.0f, 0.25f, 0.1f), factor*0.25f));

	// Camera
	int width, height;
	glfwGetWindowSize (windowPtr, &width, &height);
	auto cameraPtr = std::make_shared<Camera> ();
	cameraPtr->setAspectRatio (static_cast<float>(width) / static_cast<float>(height));
	cameraPtr->setTranslation (center + glm::vec3 (0.0, 0.0, 3.0 * meshScale));
	cameraPtr->setNear (0.1f);
	cameraPtr->setFar (100.f * meshScale);
	scenePtr->set (cameraPtr);
}

void init () {
	initGLFW (); // Windowing system
	if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress)) // Load extensions for modern OpenGL
		exitOnCriticalError ("[Failed to initialize OpenGL context]");
	initScene (); // Actual scene to render
	rasterizerPtr = make_shared<Rasterizer> ();
	rasterizerPtr->init (basePath, scenePtr); // Mut be called before creating the scene, to generate an OpenGL context and allow mesh VBOs
	rayTracerPtr = make_shared<RayTracer> ();
	rayTracerPtr->init (scenePtr);
}

void clear () {
	glfwDestroyWindow (windowPtr);
	glfwTerminate ();
}


// The main rendering call
void render () {
	if (isDisplayRaytracing)
		//rasterizerPtr->display (rayTracerPtr->image ());
		rasterizerPtr->renderSSR (scenePtr, diagnostic);
	else
		rasterizerPtr->render (scenePtr, diagnostic);
}

// Update any accessible variable based on the current time
void update (float currentTime) {
	// Animate any entity of the program here
	static const float initialTime = currentTime;
	static float lastTime = 0.f;
	static unsigned int frameCount = 0;
	static float fpsTime = currentTime;
	static unsigned int FPS = 0;
	float elapsedTime = currentTime - initialTime;
	float dt = currentTime - lastTime;
	if (frameCount == 99) {
		float delai = (currentTime - fpsTime)/100;
		FPS = static_cast<unsigned int> (1.f/delai);
		frameCount = 0;
		fpsTime = currentTime;
	}
	std::string titleWithFPS = BASE_WINDOW_TITLE + " - " + std::to_string (FPS) + "FPS";
	glfwSetWindowTitle (windowPtr, titleWithFPS.c_str ());
	lastTime = currentTime;
	frameCount++;
}

void usage (const char * command) {
	Console::print ("Usage : " + std::string(command) + " [<meshfile.off>]");
	std::exit (EXIT_FAILURE);
}

void parseCommandLine (int argc, char ** argv) {
	if (argc > 3)
		usage (argv[0]);
	fs::path appPath = argv[0];
	basePath = appPath.parent_path().string(); 
	meshFilename = basePath + "/" + (argc >= 2 ? argv[1] : ".\\Resources\\Models\\sphere_high_res.off");
}

int main (int argc, char ** argv) {
	parseCommandLine (argc, argv);
	init (); 
	while (!glfwWindowShouldClose (windowPtr)) {
		update (static_cast<float> (glfwGetTime ()));
		render ();
		glfwSwapBuffers (windowPtr);
		glfwPollEvents ();
	}
	clear ();
	Console::print ("Quit");
	return EXIT_SUCCESS;
}

