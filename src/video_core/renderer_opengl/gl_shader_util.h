// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include <glad/glad.h>

namespace GLShader {

/**
 * Utility function to create and compile an OpenGL GLSL shader
 * @param source String of the GLSL shader program
 * @param type Type of the shader (GL_VERTEX_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER)
 * @param debug_type A string for display the shader type. This is for debug only
 */
GLuint LoadShader(const char* source, GLenum type, const char* debug_type);

/**
 * Utility function to create and compile an OpenGL GLSL shader program (vertex + fragment shader)
 * @param vertex_shader ID of vertex shader
 * @param geometry_shader ID of geometry shader
 * @param fragment_shader ID of fragment shader
 * @returns Handle of the newly created OpenGL program object
 */
GLuint LoadProgram(GLuint vertex_shader, GLuint geometry_shader, GLuint fragment_shader,
                   bool separable_program = false);

} // namespace GLShader
