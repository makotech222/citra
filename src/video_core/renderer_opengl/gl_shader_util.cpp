// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_opengl/gl_shader_util.h"

namespace GLShader {

GLuint LoadShader(const char* source, GLenum type, const char* debug_type) {
    GLuint shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source, nullptr);
    NGLOG_DEBUG(Render_OpenGL, "Compiling {} shader...", debug_type);
    glCompileShader(shader_id);

    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::string shader_error(info_log_length, ' ');
        glGetShaderInfoLog(shader_id, info_log_length, nullptr, &shader_error[0]);
        if (result == GL_TRUE) {
            NGLOG_DEBUG(Render_OpenGL, "{}", shader_error);
        } else {
            NGLOG_ERROR(Render_OpenGL, "Error compiling {} shader:\n{}", debug_type, shader_error);
        }
    }
    return shader_id;
}

GLuint LoadProgram(GLuint vertex_shader_id, GLuint geometry_shader_id, GLuint fragment_shader_id,
                   bool separable_program) {
    // Link the program
    NGLOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();
    if (vertex_shader_id != 0) {
        glAttachShader(program_id, vertex_shader_id);
    }
    if (geometry_shader_id != 0) {
        glAttachShader(program_id, geometry_shader_id);
    }
    if (fragment_shader_id != 0) {
        glAttachShader(program_id, fragment_shader_id);
    }

    if (separable_program) {
        glProgramParameteri(program_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
    }

    glLinkProgram(program_id);

    // Check the program
    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::string program_error(info_log_length, ' ');
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            NGLOG_DEBUG(Render_OpenGL, "{}", program_error);
        } else {
            NGLOG_ERROR(Render_OpenGL, "Error linking shader:\n{}", program_error);
        }
    }

    ASSERT_MSG(result == GL_TRUE, "Shader not linked");

    if (vertex_shader_id != 0) {
        glDetachShader(program_id, vertex_shader_id);
    }
    if (geometry_shader_id != 0) {
        glDetachShader(program_id, geometry_shader_id);
    }
    if (fragment_shader_id != 0) {
        glDetachShader(program_id, fragment_shader_id);
    }

    return program_id;
}

} // namespace GLShader
