// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/renderer_opengl/gl_shader_manager.h"

namespace Impl {
void SetShaderUniformBlockBinding(GLuint shader, const char* name, UniformBindings binding,
                                  size_t expected_size) {
    GLuint ub_index = glGetUniformBlockIndex(shader, name);
    if (ub_index != GL_INVALID_INDEX) {
        GLint ub_size = 0;
        glGetActiveUniformBlockiv(shader, ub_index, GL_UNIFORM_BLOCK_DATA_SIZE, &ub_size);
        ASSERT_MSG(ub_size == expected_size,
                   "Uniform block size did not match! Got %d, expected %zu",
                   static_cast<int>(ub_size), expected_size);
        glUniformBlockBinding(shader, ub_index, static_cast<GLuint>(binding));
    }
}

void SetShaderUniformBlockBindings(GLuint shader) {
    SetShaderUniformBlockBinding(shader, "shader_data", UniformBindings::Common,
                                 sizeof(UniformData));
    SetShaderUniformBlockBinding(shader, "vs_config", UniformBindings::VS, sizeof(VSUniformData));
    SetShaderUniformBlockBinding(shader, "gs_config", UniformBindings::GS, sizeof(GSUniformData));
}

void SetShaderSamplerBinding(GLuint shader, const char* name, TextureUnits::TextureUnit binding) {
    GLint uniform_tex = glGetUniformLocation(shader, name);
    if (uniform_tex != -1) {
        glUniform1i(uniform_tex, binding.id);
    }
}

void SetShaderSamplerBindings(GLuint shader) {
    OpenGLState cur_state = OpenGLState::GetCurState();
    GLuint old_program = std::exchange(cur_state.draw.shader_program, shader);
    cur_state.Apply();

    // Set the texture samplers to correspond to different texture units
    SetShaderSamplerBinding(shader, "tex0", TextureUnits::PicaTexture(0));
    SetShaderSamplerBinding(shader, "tex1", TextureUnits::PicaTexture(1));
    SetShaderSamplerBinding(shader, "tex2", TextureUnits::PicaTexture(2));
    SetShaderSamplerBinding(shader, "tex_cube", TextureUnits::TextureCube);

    // Set the texture samplers to correspond to different lookup table texture units
    SetShaderSamplerBinding(shader, "lighting_lut", TextureUnits::LightingLUT);
    SetShaderSamplerBinding(shader, "fog_lut", TextureUnits::FogLUT);
    SetShaderSamplerBinding(shader, "proctex_noise_lut", TextureUnits::ProcTexNoiseLUT);
    SetShaderSamplerBinding(shader, "proctex_color_map", TextureUnits::ProcTexColorMap);
    SetShaderSamplerBinding(shader, "proctex_alpha_map", TextureUnits::ProcTexAlphaMap);
    SetShaderSamplerBinding(shader, "proctex_lut", TextureUnits::ProcTexLUT);
    SetShaderSamplerBinding(shader, "proctex_diff_lut", TextureUnits::ProcTexDiffLUT);

    cur_state.draw.shader_program = old_program;
    cur_state.Apply();
}

} // namespace Impl

void PicaUniformsData::SetFromRegs(const Pica::ShaderRegs& regs,
                                   const Pica::Shader::ShaderSetup& setup) {
    for (size_t it = 0; it < 16; ++it) {
        bools[it].b = setup.uniforms.b[it] ? GL_TRUE : GL_FALSE;
    }
    for (size_t it = 0; it < 4; ++it) {
        i[it][0] = regs.int_uniforms[it].x;
        i[it][1] = regs.int_uniforms[it].y;
        i[it][2] = regs.int_uniforms[it].z;
        i[it][3] = regs.int_uniforms[it].w;
    }
    std::memcpy(&f[0], &setup.uniforms.f[0], sizeof(f));
}
