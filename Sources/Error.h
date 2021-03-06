// ----------------------------------------------
// Polytechnique - INF584 "Image Synthesis"
//
// Base code for practical assignments.
//
// Copyright (C) 2022 Tamy Boubekeur
// All rights reserved.
// ----------------------------------------------
#pragma once
#include <glad/glad.h>
#include <string>

/// Executed whenever a debug message is generated by OpenGL, including the critical errors.
void APIENTRY debugMessageCallback (GLenum source,
        				   			GLenum type,
        				   			GLuint id,
        				   			GLenum severity,
        				   			GLsizei length,
        				   			const GLchar* message,
        				   			const void* userParam);

void exitOnCriticalError (const std::string & message);


static int printOglError (const std::string & msg, const char * file, int line) {

    GLenum glErr;

    int    retCode = 0;

    glErr = glGetError ();

    while (glErr != GL_NO_ERROR) {

        printf ("glError in file %s @ line %d: %s - %s\n", file, line, "None", msg.c_str ());

        retCode = 1;

        glErr = glGetError ();

    }

    return retCode;

}

#define printOpenGLError(MSG) printOglError ((MSG), __FILE__, __LINE__)

