// ----------------------------------------------
// Polytechnique - INF584 "Image Synthesis"
//
// Base code for practical assignments.
//
// Copyright (C) 2022 Tamy Boubekeur
// All rights reserved.
// ----------------------------------------------
#include "Rasterizer.h"
#include <glad/glad.h>
#include "Resources.h"
#include "Error.h"

void Rasterizer::init (const std::string & basePath, const std::shared_ptr<Scene> scenePtr) {
	glEnable (GL_DEBUG_OUTPUT); // Modern error callback functionnality
	glEnable (GL_DEBUG_OUTPUT_SYNCHRONOUS); // For recovering the line where the error occurs, set a debugger breakpoint in DebugMessageCallback
    glDebugMessageCallback (debugMessageCallback, 0); // Specifies the function to call when an error message is generated.
	glCullFace (GL_BACK);     // Specifies the faces to cull (here the ones pointing away from the camera)
	glEnable (GL_CULL_FACE); // Enables face culling (based on the orientation defined by the CW/CCW enumeration).
	glDepthFunc (GL_LESS); // Specify the depth test for the z-buffer
	glEnable (GL_DEPTH_TEST); // Enable the z-buffer test in the rasterization
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f); // specify the background color, used any time the framebuffer is cleared
	// GPU resources
	initScreeQuad ();
	loadShaderProgram (basePath);
	initDisplayedImage ();
	// Allocate GPU ressources for the heavy data components of the scene 
	size_t numOfMeshes = scenePtr->numOfMeshes ();
	for (size_t i = 0; i < numOfMeshes; i++) 
		m_vaos.push_back (toGPU (scenePtr->mesh (i)));
}

void Rasterizer::setResolution (int width, int height)  {
	glViewport (0, 0, (GLint)width, (GLint)height); // Dimension of the rendering region in the window
}

void Rasterizer::loadShaderProgram (const std::string & basePath) {
	m_pbrShaderProgramPtr.reset ();
	try {
		std::string shaderPath = basePath + "/" + SHADER_PATH;
		m_pbrShaderProgramPtr = ShaderProgram::genBasicShaderProgram (shaderPath + "/PBRVertexShader.glsl",
													         	 	  shaderPath + "/PBRFragmentShader.glsl");
	} catch (std::exception & e) {
		exitOnCriticalError (std::string ("[Error loading shader program]") + e.what ());
	}
	m_displayShaderProgramPtr.reset ();
	try {
		std::string shaderPath = basePath + "/" + SHADER_PATH;
		m_displayShaderProgramPtr = ShaderProgram::genBasicShaderProgram (shaderPath + "/DisplayVertexShader.glsl",
													         	 		  shaderPath + "/DisplayFragmentShader.glsl");
		m_displayShaderProgramPtr->set ("imageTex", 0);
	} catch (std::exception & e) {
		exitOnCriticalError (std::string ("[Error loading display shader program]") + e.what ());
	}
}

void Rasterizer::updateDisplayedImageTexture (std::shared_ptr<Image> imagePtr) {
	glBindTexture (GL_TEXTURE_2D, m_displayImageTex);
   	// Uploading the image data to GPU memory
	glTexImage2D (
		GL_TEXTURE_2D, 
		0, 
   		GL_RGB, // We assume only greyscale or RGB pixels
   		static_cast<GLsizei> (imagePtr->width()), 
   		static_cast<GLsizei> (imagePtr->height()), 
   		0, 
   		GL_RGB, // We assume only greyscale or RGB pixels
   		GL_FLOAT, 
   		imagePtr->pixels().data());
   	// Generating mipmaps for filtered texture fetch
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, 0);
}

void Rasterizer::initDisplayedImage () {
	// Creating and configuring the GPU texture that will contain the image to display
	glGenTextures (1, &m_displayImageTex);
	glBindTexture (GL_TEXTURE_2D, m_displayImageTex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture (GL_TEXTURE_2D, 0);
}


void Rasterizer::renderSSR (std::shared_ptr<Scene> scenePtr) {
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

	std::shared_ptr<ShaderProgram> m_SSRFirstPassShaderProgramPtr;
	std::string basePath = "C:\\Users\\josse\\Documents\\INF584\\INF584_MyRenderer\\Resources\\Shaders\\";
	try {
		m_SSRFirstPassShaderProgramPtr = ShaderProgram::genBasicShaderProgram (basePath + "SSRFirstPassVertexShader.glsl",
													         	 	  			basePath + "SSRFirstPassFragmentShader.glsl");
	} catch (std::exception & e) {
		exitOnCriticalError (std::string ("[Error loading shader program]") + e.what ());
	}

	std::shared_ptr<ShaderProgram> m_SSRShaderProgramPtr;
	try {
		m_SSRShaderProgramPtr = ShaderProgram::genBasicShaderProgram (basePath + "SSRVertexShader.glsl",
													         	 	  	basePath + "SSRFragmentShader.glsl");
	} catch (std::exception & e) {
		exitOnCriticalError (std::string ("[Error loading shader program]") + e.what ());
	}

	//std::cout << "Hey before use shader 1" << std::endl;
	m_SSRFirstPassShaderProgramPtr->use();
	//std::cout << "Hey after use shader 1" << std::endl;

	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);



	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures(1, &renderedTexture);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, 1024, 768, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
	//glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT24, 1024, 768, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// Poor filtering. Needed !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	// The depth buffer
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 768);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

	// Always check that our framebuffer is ok
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	return;

		// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
	glViewport(0,0,1024,768); // Render on the whole framebuffer, complete from the lower left corner to the upper right

	//std::cout << "Hey before render" << std::endl;
	render(scenePtr);
	//std::cout << "Hey after render" << std::endl;





	
	// Create and compile our GLSL program from the shaders
	//std::cout << "Hey before use shader 2" << std::endl;
	m_SSRShaderProgramPtr->use();
	//std::cout << "Hey after use shader 2" << std::endl;



	// The fullscreen quad's FBO
	GLuint quad_VertexArrayID;
	glGenVertexArrays(1, &quad_VertexArrayID);
	glBindVertexArray(quad_VertexArrayID);

	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};
	//std::cout << "Hey after def quad" << std::endl;

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	//std::cout << "Hey after gen array" << std::endl;

	GLuint texID = glGetUniformLocation(m_SSRShaderProgramPtr->id(), "renderedTexture");
	GLuint timeID = glGetUniformLocation(m_SSRShaderProgramPtr->id(), "time");

	// Render to the screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,1024,768); // Render on the whole framebuffer, complete from the lower left corner to the upper right
	glBindVertexArray (m_screenQuadVao); // Activate the VAO storing geometry data
	glDrawElements (GL_TRIANGLES, static_cast<GLsizei> (6), GL_UNSIGNED_INT, 0);
	m_SSRShaderProgramPtr->stop ();
}



// The main rendering call
void Rasterizer::render (std::shared_ptr<Scene> scenePtr) {
	const glm::vec3 & bgColor = scenePtr->backgroundColor ();
	glClearColor (bgColor[0], bgColor[1], bgColor[2], 1.f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.
	

	// Light Source - DIRECTIONNAL
	size_t numOfLightSourcesDir = scenePtr->numOfLightSourcesDir();
	for(size_t i=0; i<numOfLightSourcesDir; i++) {
		auto lightSourcePtr = scenePtr->lightSourceDir(i);
		m_pbrShaderProgramPtr->set ("lightsourcesDir[" + std::to_string(i) + "].direction", lightSourcePtr->direction);
		m_pbrShaderProgramPtr->set ("lightsourcesDir[" + std::to_string(i) + "].intensity", lightSourcePtr->intensity);
		m_pbrShaderProgramPtr->set ("lightsourcesDir[" + std::to_string(i) + "].color", lightSourcePtr->color);
	}
	m_pbrShaderProgramPtr->set ("nb_lightsourcesDir", (int)(numOfLightSourcesDir));


	// Light Source - PONCTUAL
	size_t numOfLightSourcesPoint = scenePtr->numOflightSourcesPoint();
	for(size_t i=0; i<numOfLightSourcesPoint; i++) {
		auto lightSourcePtr = scenePtr->lightSourcePoint(i);
		m_pbrShaderProgramPtr->set ("lightsourcesPoint[" + std::to_string(i) + "].position", lightSourcePtr->position);
		m_pbrShaderProgramPtr->set ("lightsourcesPoint[" + std::to_string(i) + "].intensity", lightSourcePtr->intensity);
		m_pbrShaderProgramPtr->set ("lightsourcesPoint[" + std::to_string(i) + "].color", lightSourcePtr->color);
		m_pbrShaderProgramPtr->set ("lightsourcesPoint[" + std::to_string(i) + "].a_c", lightSourcePtr->a_c);
		m_pbrShaderProgramPtr->set ("lightsourcesPoint[" + std::to_string(i) + "].a_l", lightSourcePtr->a_l);
		m_pbrShaderProgramPtr->set ("lightsourcesPoint[" + std::to_string(i) + "].a_q", lightSourcePtr->a_q);
	}
	m_pbrShaderProgramPtr->set ("nb_lightsourcesPoint", (int)(numOfLightSourcesPoint));


	// Meshes
	size_t numOfMeshes = scenePtr->numOfMeshes ();
	for (size_t i = 0; i < numOfMeshes; i++) {
		glm::mat4 projectionMatrix = scenePtr->camera()->computeProjectionMatrix ();
		m_pbrShaderProgramPtr->set ("projectionMat", projectionMatrix); // Compute the projection matrix of the camera and pass it to the GPU program
		glm::mat4 modelMatrix = scenePtr->mesh (i)->computeTransformMatrix ();
		glm::mat4 viewMatrix = scenePtr->camera()->computeViewMatrix ();
		m_pbrShaderProgramPtr->set ("viewMat", viewMatrix);
		glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;
		glm::mat4 normalMatrix = glm::transpose (glm::inverse (modelViewMatrix));
		m_pbrShaderProgramPtr->set ("modelViewMat", modelViewMatrix);
		m_pbrShaderProgramPtr->set ("normalMat", normalMatrix);

		// Material
		size_t materialIndex = scenePtr->getMaterialOfMesh(0);
		auto materialPtr = scenePtr->material(materialIndex);
		m_pbrShaderProgramPtr->set ("material.albedo", materialPtr->albedo ());
		m_pbrShaderProgramPtr->set ("material.roughness", materialPtr->roughness ());
		m_pbrShaderProgramPtr->set ("material.metallicness", materialPtr->metallicness ());

		draw (i, scenePtr->mesh (i)->triangleIndices().size ());
	}

	/*
	size_t numOfMaterials = scenePtr->numOfMaterials ();
	for (size_t i = 0; i < numOfMaterials; i++) {
		auto materialPtr = scenePtr->material(i);

		m_pbrShaderProgramPtr->set ("material.albedo", materialPtr->albedo ());
		m_pbrShaderProgramPtr->set ("material.roughness", materialPtr->roughness ());
		m_pbrShaderProgramPtr->set ("material.metallicness", materialPtr->metallicness ());
	}*/

	m_pbrShaderProgramPtr->stop ();
	
}

void Rasterizer::display (std::shared_ptr<Image> imagePtr) {
	updateDisplayedImageTexture (imagePtr);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.
	m_displayShaderProgramPtr->use (); // Activate the program to be used for upcoming primitive
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, m_displayImageTex);
	glBindVertexArray (m_screenQuadVao); // Activate the VAO storing geometry data
	glDrawElements (GL_TRIANGLES, static_cast<GLsizei> (6), GL_UNSIGNED_INT, 0);
	m_displayShaderProgramPtr->stop ();
}

void Rasterizer::clear () {
	for (unsigned int i = 0; i < m_posVbos.size (); i++) {
		GLuint vbo = m_posVbos[i];
		glDeleteBuffers (1, &vbo);
	}
	m_posVbos.clear ();
	for (unsigned int i = 0; i < m_normalVbos.size (); i++) {
		GLuint vbo = m_normalVbos[i];
		glDeleteBuffers (1, &vbo);
	}
	m_normalVbos.clear ();
	for (unsigned int i = 0; i < m_texCoordsVbos.size (); i++) {
		GLuint vbo = m_texCoordsVbos[i];
		glDeleteBuffers (1, &vbo);
	}
	m_texCoordsVbos.clear ();
	for (unsigned int i = 0; i < m_ibos.size (); i++) {
		GLuint ibo = m_ibos[i];
		glDeleteBuffers (1, &ibo);
	}
	m_ibos.clear ();
	for (unsigned int i = 0; i < m_vaos.size (); i++) {
		GLuint vao = m_vaos[i];
		glDeleteVertexArrays (1, &vao);
	}
	m_vaos.clear ();
}

GLuint Rasterizer::genGPUBuffer (size_t elementSize, size_t numElements, const void * data) {
	GLuint vbo;
	glCreateBuffers (1, &vbo); // Generate a GPU buffer to store the positions of the vertices
	size_t size = elementSize * numElements; // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage (vbo, size, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data store on the GPU
	glNamedBufferSubData (vbo, 0, size, data); // Fill the data store from a CPU array
	return vbo;
}

GLuint Rasterizer::genGPUVertexArray (GLuint posVbo, GLuint ibo, bool hasNormals, GLuint normalVbo, GLuint texCoordsVbo) {
	GLuint vao;
	glCreateVertexArrays (1, &vao); // Create a single handle that joins together attributes (vertex positions, normals) and connectivity (triangles indices)
	glBindVertexArray (vao);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, posVbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat), 0);
	if (hasNormals) {
		glEnableVertexAttribArray (1);
		glBindBuffer (GL_ARRAY_BUFFER, normalVbo);
		glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat), 0);

		// UV
		glEnableVertexAttribArray(2);
		glBindBuffer (GL_ARRAY_BUFFER, texCoordsVbo);
		glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (GLfloat), 0);
	}
	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBindVertexArray (0); // Desactive the VAO just created. Will be activated at rendering time.
	return vao;
}


GLuint Rasterizer::toGPU (std::shared_ptr<Mesh> meshPtr) {
	GLuint posVbo = genGPUBuffer (3 * sizeof (float), meshPtr->vertexPositions().size(), meshPtr->vertexPositions().data ()); // Position GPU vertex buffer
	GLuint normalVbo = genGPUBuffer (3 * sizeof (float), meshPtr->vertexNormals().size(), meshPtr->vertexNormals().data ()); // Normal GPU vertex buffer
	GLuint texPosVbo = genGPUBuffer (2 * sizeof (float), meshPtr->vertexTexCoords().size(), meshPtr->vertexTexCoords().data ()); // Normal GPU vertex buffer
	GLuint ibo = genGPUBuffer (sizeof (glm::uvec3), meshPtr->triangleIndices().size(), meshPtr->triangleIndices().data ()); // triangle GPU index buffer
	GLuint vao = genGPUVertexArray (posVbo, ibo, true, normalVbo, texPosVbo);
	return vao;
}

void Rasterizer::initScreeQuad () {
	std::vector<float> pData = {-1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0};
	std::vector<unsigned int> iData = {0, 1, 2, 0, 2, 3};
	m_screenQuadVao = genGPUVertexArray (
		genGPUBuffer (3*sizeof(float), 4, pData.data()),
		genGPUBuffer (3*sizeof(unsigned int), 2, iData.data()),
		genGPUBuffer (2*sizeof(unsigned int), 2, iData.data()),
		false,
		0
	);
}

void Rasterizer::draw (size_t meshId, size_t triangleCount) {
	glBindVertexArray (m_vaos[meshId]); // Activate the VAO storing geometry data
	glDrawElements (GL_TRIANGLES, static_cast<GLsizei> (triangleCount * 3), GL_UNSIGNED_INT, 0); // Call for rendering: stream the current GPU geometry through the current GPU program
}
