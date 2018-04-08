// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>
#include "video_core/renderer_opengl/gl_shader_manager.h"

static void SetShaderUniformBlockBinding(GLuint shader, const char* name, UniformBindings binding,
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

static void SetShaderUniformBlockBindings(GLuint shader) {
    SetShaderUniformBlockBinding(shader, "shader_data", UniformBindings::Common,
                                 sizeof(UniformData));
    SetShaderUniformBlockBinding(shader, "vs_config", UniformBindings::VS, sizeof(VSUniformData));
    SetShaderUniformBlockBinding(shader, "gs_config", UniformBindings::GS, sizeof(GSUniformData));
}

static void SetShaderSamplerBinding(GLuint shader, const char* name,
                                    TextureUnits::TextureUnit binding) {
    GLint uniform_tex = glGetUniformLocation(shader, name);
    if (uniform_tex != -1) {
        glUniform1i(uniform_tex, binding.id);
    }
}

static void SetShaderSamplerBindings(GLuint shader) {
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

class OGLShaderStage {
public:
    OGLShaderStage(bool separable) {
        if (separable) {
            shader_or_program = OGLProgram();
        } else {
            shader_or_program = OGLShader();
        }
    }
    void Create(const char* source, GLenum type) {
        if (shader_or_program.which() == 0) {
            boost::get<OGLShader>(shader_or_program).Create(source, type);
        } else {
            OGLShader shader;
            shader.Create(source, type);
            OGLProgram& program = boost::get<OGLProgram>(shader_or_program);
            program.Create(true, {shader.handle});
            SetShaderUniformBlockBindings(program.handle);
            SetShaderSamplerBindings(program.handle);
        }
    }
    GLuint GetHandle() const {
        if (shader_or_program.which() == 0) {
            return boost::get<OGLShader>(shader_or_program).handle;
        } else {
            return boost::get<OGLProgram>(shader_or_program).handle;
        }
    }

private:
    boost::variant<OGLShader, OGLProgram> shader_or_program;
};

class DefaultVertexShader {
public:
    DefaultVertexShader(bool separable) : program(separable) {
        program.Create(GLShader::GenerateDefaultVertexShader(separable).c_str(), GL_VERTEX_SHADER);
    }
    GLuint Get() {
        return program.GetHandle();
    }

private:
    OGLShaderStage program;
};

template <typename KeyConfigType, std::string (*CodeGenerator)(const KeyConfigType&, bool),
          GLenum ShaderType>
class ShaderCache {
public:
    ShaderCache(bool separable) : separable(separable) {}
    GLuint Get(const KeyConfigType& config) {
        auto [iter, new_shader] = shaders.emplace(config, OGLShaderStage{separable});
        OGLShaderStage& cached_shader = iter->second;
        if (new_shader) {
            cached_shader.Create(CodeGenerator(config, separable).c_str(), ShaderType);
        }
        return cached_shader.GetHandle();
    }

private:
    bool separable;
    std::unordered_map<KeyConfigType, OGLShaderStage> shaders;
};

// TODO(wwylele): beautify this doc
// This is a shader cache designed for translating PICA shader to GLSL shader.
// The double cache is needed because diffent KeyConfigType, which includes a hash of the code
// region (including its leftover unused code) can generate the same GLSL code.
template <typename KeyConfigType,
          std::string (*CodeGenerator)(const Pica::Shader::ShaderSetup&, const KeyConfigType&,
                                       bool),
          GLenum ShaderType>
class ShaderDoubleCache {
public:
    ShaderDoubleCache(bool separable) : separable(separable) {}
    GLuint Get(const KeyConfigType& key, const Pica::Shader::ShaderSetup& setup) {
        auto map_it = shader_map.find(key);
        if (map_it == shader_map.end()) {
            std::string program = CodeGenerator(setup, key, separable);

            auto [iter, new_shader] = shader_cache.emplace(program, OGLShaderStage{separable});
            OGLShaderStage& cached_shader = iter->second;
            if (new_shader) {
                cached_shader.Create(program.c_str(), ShaderType);
            }
            shader_map[key] = &cached_shader;
            return cached_shader.GetHandle();
        } else {
            return map_it->second->GetHandle();
        }
    }

private:
    bool separable;
    std::unordered_map<KeyConfigType, OGLShaderStage*> shader_map;
    std::unordered_map<std::string, OGLShaderStage> shader_cache;
};

using ProgrammableVertexShaders =
    ShaderDoubleCache<GLShader::PicaVSConfig, &GLShader::GenerateVertexShader, GL_VERTEX_SHADER>;

using ProgrammableGeometryShaders =
    ShaderDoubleCache<GLShader::PicaGSConfig, &GLShader::GenerateGeometryShader,
                      GL_GEOMETRY_SHADER>;

using FixedGeometryShaders =
    ShaderCache<GLShader::PicaGSConfigCommon, &GLShader::GenerateDefaultGeometryShader,
                GL_GEOMETRY_SHADER>;

using FragmentShaders =
    ShaderCache<GLShader::PicaShaderConfig, &GLShader::GenerateFragmentShader, GL_FRAGMENT_SHADER>;

class ShaderProgramManager::Impl {
public:
    Impl(bool separable)
        : separable(separable), programmable_vertex_shaders(separable),
          default_vertex_shader(separable), programmable_geometry_shaders(separable),
          fixed_geometry_shaders(separable), fragment_shaders(separable) {
        if (separable)
            pipeline.Create();
    }

    struct ShaderTuple {
        GLuint vs = 0, gs = 0, fs = 0;
        bool operator==(const ShaderTuple& rhs) const {
            return std::tie(vs, gs, fs) == std::tie(rhs.vs, rhs.gs, rhs.fs);
        }
        struct Hash {
            std::size_t operator()(const ShaderTuple& tuple) const {
                std::size_t hash = 0;
                boost::hash_combine(hash, tuple.vs);
                boost::hash_combine(hash, tuple.gs);
                boost::hash_combine(hash, tuple.fs);
                return hash;
            }
        };
    };

    ShaderTuple current;

    ProgrammableVertexShaders programmable_vertex_shaders;
    DefaultVertexShader default_vertex_shader;

    ProgrammableGeometryShaders programmable_geometry_shaders;
    FixedGeometryShaders fixed_geometry_shaders;

    FragmentShaders fragment_shaders;

    bool separable;
    std::unordered_map<ShaderTuple, OGLProgram, ShaderTuple::Hash> program_cache;
    OGLPipeline pipeline;
};

ShaderProgramManager::ShaderProgramManager(bool separable)
    : impl(std::make_unique<Impl>(separable)) {}

ShaderProgramManager::~ShaderProgramManager() = default;

void ShaderProgramManager::UseProgrammableVertexShader(const GLShader::PicaVSConfig& config,
                                                       const Pica::Shader::ShaderSetup setup) {
    impl->current.vs = impl->programmable_vertex_shaders.Get(config, setup);
}

void ShaderProgramManager::UseTrivialVertexShader() {
    impl->current.vs = impl->default_vertex_shader.Get();
}

void ShaderProgramManager::UseProgrammableGeometryShader(const GLShader::PicaGSConfig& config,
                                                         const Pica::Shader::ShaderSetup setup) {
    impl->current.gs = impl->programmable_geometry_shaders.Get(config, setup);
}

void ShaderProgramManager::UseFixedGeometryShader(const GLShader::PicaGSConfigCommon& config) {
    impl->current.gs = impl->fixed_geometry_shaders.Get(config);
}

void ShaderProgramManager::UseTrivialGeometryShader() {
    impl->current.gs = 0;
}

void ShaderProgramManager::UseFragmentShader(const GLShader::PicaShaderConfig& config) {
    impl->current.fs = impl->fragment_shaders.Get(config);
}

void ShaderProgramManager::ApplyTo(OpenGLState& state) {
    if (impl->separable) {
        // Workaround for AMD bug
        glUseProgramStages(impl->pipeline.handle,
                           GL_VERTEX_SHADER_BIT | GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT,
                           0);

        glUseProgramStages(impl->pipeline.handle, GL_VERTEX_SHADER_BIT, impl->current.vs);
        glUseProgramStages(impl->pipeline.handle, GL_GEOMETRY_SHADER_BIT, impl->current.gs);
        glUseProgramStages(impl->pipeline.handle, GL_FRAGMENT_SHADER_BIT, impl->current.fs);
        state.draw.shader_program = 0;
        state.draw.program_pipeline = impl->pipeline.handle;
    } else {
        OGLProgram& cached_program = impl->program_cache[impl->current];
        if (cached_program.handle == 0) {
            cached_program.Create(false, {impl->current.vs, impl->current.gs, impl->current.fs});
            SetShaderUniformBlockBindings(cached_program.handle);
            SetShaderSamplerBindings(cached_program.handle);
        }
        state.draw.shader_program = cached_program.handle;
    }
}
