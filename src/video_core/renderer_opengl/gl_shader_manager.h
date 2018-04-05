// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <tuple>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>
#include <glad/glad.h>
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"
#include "video_core/renderer_opengl/pica_to_gl.h"

namespace Impl {
void SetShaderUniformBlockBindings(GLuint shader);
void SetShaderSamplerBindings(GLuint shader);
} // namespace Impl

enum class UniformBindings : GLuint { Common, VS, GS };

struct LightSrc {
    alignas(16) GLvec3 specular_0;
    alignas(16) GLvec3 specular_1;
    alignas(16) GLvec3 diffuse;
    alignas(16) GLvec3 ambient;
    alignas(16) GLvec3 position;
    alignas(16) GLvec3 spot_direction; // negated
    GLfloat dist_atten_bias;
    GLfloat dist_atten_scale;
};

/// Uniform structure for the Uniform Buffer Object, all vectors must be 16-byte aligned
// NOTE: Always keep a vec4 at the end. The GL spec is not clear wether the alignment at
//       the end of a uniform block is included in UNIFORM_BLOCK_DATA_SIZE or not.
//       Not following that rule will cause problems on some AMD drivers.
struct UniformData {
    GLint framebuffer_scale;
    GLint alphatest_ref;
    GLfloat depth_scale;
    GLfloat depth_offset;
    GLint scissor_x1;
    GLint scissor_y1;
    GLint scissor_x2;
    GLint scissor_y2;
    alignas(16) GLvec3 fog_color;
    alignas(8) GLvec2 proctex_noise_f;
    alignas(8) GLvec2 proctex_noise_a;
    alignas(8) GLvec2 proctex_noise_p;
    alignas(16) GLvec3 lighting_global_ambient;
    LightSrc light_src[8];
    alignas(16) GLvec4 const_color[6]; // A vec4 color for each of the six tev stages
    alignas(16) GLvec4 tev_combiner_buffer_color;
    alignas(16) GLvec4 clip_coef;
};

static_assert(
    sizeof(UniformData) == 0x460,
    "The size of the UniformData structure has changed, update the structure in the shader");
static_assert(sizeof(UniformData) < 16384,
              "UniformData structure must be less than 16kb as per the OpenGL spec");

struct PicaUniformsData {
    void SetFromRegs(const Pica::ShaderRegs& regs, const Pica::Shader::ShaderSetup& setup);

    struct {
        alignas(16) GLuint b;
    } bools[16];
    alignas(16) std::array<GLuvec4, 4> i;
    alignas(16) std::array<GLvec4, 96> f;
};

struct VSUniformData {
    PicaUniformsData uniforms;
};
static_assert(
    sizeof(VSUniformData) == 1856,
    "The size of the VSUniformData structure has changed, update the structure in the shader");
static_assert(sizeof(VSUniformData) < 16384,
              "VSUniformData structure must be less than 16kb as per the OpenGL spec");

struct GSUniformData {
    PicaUniformsData uniforms;
};
static_assert(
    sizeof(GSUniformData) == 1856,
    "The size of the GSUniformData structure has changed, update the structure in the shader");
static_assert(sizeof(GSUniformData) < 16384,
              "GSUniformData structure must be less than 16kb as per the OpenGL spec");

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
            program.Create(true, shader.handle);
            Impl::SetShaderUniformBlockBindings(program.handle);
            Impl::SetShaderSamplerBindings(program.handle);
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

/*template <class... Ts>
class ComposeShaderGetter : private Ts... {
public:
    using Ts::Get...;
    ComposeShaderGetter(bool separable) : Ts(separable)... {}
};*/

template <class... Ts>
class ComposeShaderGetter;

template <>
class ComposeShaderGetter<> {
public:
    ComposeShaderGetter(bool separable) {}
    void Get() {}
};

template <class T, class... Ts>
class ComposeShaderGetter<T, Ts...> : private T, private ComposeShaderGetter<Ts...> {
public:
    using T::Get;
    using ComposeShaderGetter<Ts...>::Get;
    ComposeShaderGetter(bool separable) : T(separable), ComposeShaderGetter<Ts...>(separable) {}
};

struct DefaultVertexShaderTag {};

class DefaultVertexShader {
public:
    DefaultVertexShader(bool separable) : program(separable) {
        program.Create(GLShader::GenerateDefaultVertexShader(separable).c_str(), GL_VERTEX_SHADER);
    }
    GLuint Get(DefaultVertexShaderTag) {
        return program.GetHandle();
    }

private:
    OGLShaderStage program;
};

struct DefaultGeometryShaderTag {};

class DefaultGeometryShader {
public:
    DefaultGeometryShader(bool) {}
    GLuint Get(DefaultGeometryShaderTag) {
        return 0;
    }
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
    GLuint Get(std::tuple<const KeyConfigType&, const Pica::Shader::ShaderSetup&> config) {
        const auto& [key, setup] = config;
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

using VertexShaders = ComposeShaderGetter<ProgrammableVertexShaders, DefaultVertexShader>;

using ProgrammableGeometryShaders =
    ShaderDoubleCache<GLShader::PicaGSConfig, &GLShader::GenerateGeometryShader,
                      GL_GEOMETRY_SHADER>;

using FixedGeometryShaders =
    ShaderCache<GLShader::PicaGSConfigCommon, &GLShader::GenerateDefaultGeometryShader,
                GL_GEOMETRY_SHADER>;

using GeometryShaders =
    ComposeShaderGetter<ProgrammableGeometryShaders, FixedGeometryShaders, DefaultGeometryShader>;

using FragmentShaders =
    ShaderCache<GLShader::PicaShaderConfig, &GLShader::GenerateFragmentShader, GL_FRAGMENT_SHADER>;

class ShaderProgramManager {
public:
    ShaderProgramManager(bool separable)
        : separable(separable), vertex_shaders(separable), geometry_shaders(separable),
          fragment_shaders(separable) {
        if (separable)
            pipeline.Create();
    }

    template <typename ConfigType>
    void UseVertexShader(const ConfigType& config) {
        current.vs = vertex_shaders.Get(config);
    }

    template <typename ConfigType>
    void UseGeometryShader(const ConfigType& config) {
        current.gs = geometry_shaders.Get(config);
    }

    template <typename ConfigType>
    void UseFragmentShader(const ConfigType& config) {
        current.fs = fragment_shaders.Get(config);
    }

    void ApplyTo(OpenGLState& state) {
        if (separable) {
            // Workaround for AMD bug
            glUseProgramStages(
                pipeline.handle,
                GL_VERTEX_SHADER_BIT | GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, 0);

            glUseProgramStages(pipeline.handle, GL_VERTEX_SHADER_BIT, current.vs);
            glUseProgramStages(pipeline.handle, GL_GEOMETRY_SHADER_BIT, current.gs);
            glUseProgramStages(pipeline.handle, GL_FRAGMENT_SHADER_BIT, current.fs);
            state.draw.shader_program = 0;
            state.draw.program_pipeline = pipeline.handle;
        } else {
            OGLProgram& cached_program = program_cache[current];
            if (cached_program.handle == 0) {
                cached_program.Create(false, current.vs, current.gs, current.fs);
                Impl::SetShaderUniformBlockBindings(cached_program.handle);
                Impl::SetShaderSamplerBindings(cached_program.handle);
            }
            state.draw.shader_program = cached_program.handle;
        }
    }

private:
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
    VertexShaders vertex_shaders{false};
    GeometryShaders geometry_shaders{false};
    FragmentShaders fragment_shaders{false};

    bool separable;
    std::unordered_map<ShaderTuple, OGLProgram, ShaderTuple::Hash> program_cache;
    OGLPipeline pipeline;
};
