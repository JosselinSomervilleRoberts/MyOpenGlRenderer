#version 450 core // Minimal GL version support expected from the GPU

layout(location=0) in vec3 vPosition; // The 1st input attribute is the screen-space position (CPU side: glVertexAttrib 0)
layout(location=1) in vec2 vTexCoords; // The 1st input attribute is the screen-space position (CPU side: glVertexAttrib 0)

out vec2 TexCoords;

void main() {
    TexCoords = vTexCoords;
    gl_Position =  vec4 (vPosition, 1.0);; // mandatory to fire rasterization properly
}