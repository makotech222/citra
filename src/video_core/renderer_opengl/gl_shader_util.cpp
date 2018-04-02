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

GLuint LoadProgram(const char* vertex_shader, const char* geometry_shader,
                   const char* fragment_shader, const std::vector<const char*>& feedback_vars,
                   bool separable_program) {
    // Create the shaders
    GLuint vertex_shader_id =
        vertex_shader ? LoadShader(vertex_shader, GL_VERTEX_SHADER, "vertex") : 0;
    GLuint geometry_shader_id =
        geometry_shader ? LoadShader(geometry_shader, GL_GEOMETRY_SHADER, "geometry") : 0;
    GLuint fragment_shader_id =
        fragment_shader ? LoadShader(fragment_shader, GL_FRAGMENT_SHADER, "fragment") : 0;

    // Link the program
    LOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();
    if (vertex_shader) {
        glAttachShader(program_id, vertex_shader_id);
    }
    if (geometry_shader) {
        glAttachShader(program_id, geometry_shader_id);
    }
    if (fragment_shader) {
        glAttachShader(program_id, fragment_shader_id);
    }

    if (!feedback_vars.empty()) {
        auto varyings = feedback_vars;
        glTransformFeedbackVaryings(program_id, static_cast<GLsizei>(feedback_vars.size()),
                                    &varyings[0], GL_INTERLEAVED_ATTRIBS);
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
        std::vector<char> program_error(info_log_length);
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "%s", &program_error[0]);
        } else {
            LOG_ERROR(Render_OpenGL, "Error linking shader:\n%s", &program_error[0]);
        }
    }

    // If the program linking failed at least one of the shaders was probably bad
    if (result == GL_FALSE) {
        if (vertex_shader) {
            LOG_ERROR(Render_OpenGL, "Vertex shader:\n%s", vertex_shader);
        }
        if (geometry_shader) {
            LOG_ERROR(Render_OpenGL, "Geometry shader:\n%s", geometry_shader);
        }
        if (fragment_shader) {
            LOG_ERROR(Render_OpenGL, "Fragment shader:\n%s", fragment_shader);
        }
    }
    ASSERT_MSG(result == GL_TRUE, "Shader not linked");

    if (vertex_shader) {
        glDetachShader(program_id, vertex_shader_id);
        glDeleteShader(vertex_shader_id);
    }
    if (geometry_shader) {
        glDetachShader(program_id, geometry_shader_id);
        glDeleteShader(geometry_shader_id);
    }
    if (fragment_shader) {
        glDetachShader(program_id, fragment_shader_id);
        glDeleteShader(fragment_shader_id);
    }

    return program_id;
}

} // namespace GLShader
