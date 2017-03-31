
#include "kgl.hpp"
#include "wrappedcalls.hpp"
#include <bitset>

namespace kgl {

//
// Shader implementation
//

enum class ShaderFlag { Count };
enum class UniformType { UNIFORM, SUBROUTINE_NAME, SUBROUTINE_VALUE };
struct ShaderUniform {
    std::string name;
    GLint       location;
    UniformType type;

    ShaderUniform (const std::string& name, decltype(location) location, decltype(type) type) :
        name(name), location(location), type(type) {}
    ShaderUniform (const ShaderUniform&) = default;
};
struct SubShader {
    GLenum type;
    GLuint handle;
    ShaderStatus status = ShaderStatus::EMPTY;

    SubShader (decltype(type) type, decltype(handle) handle, decltype(status) status)
        : type(type), handle(handle), status(status) {}
    SubShader (const SubShader&) = default;
};

// Neat trick for information hiding. Note: ONLY works for leaf / final classes, since having
// hidden de-facto member variables makes further inheritance impossible. 
// BUT!, this is quite elegant and cleaner than PIMPL.
struct ShaderImpl : public Shader {
    GLuint handle;
    ShaderStatus status = ShaderStatus::EMPTY;
    std::bitset<static_cast<size_t>(ShaderFlag::Count)> flags;
    std::vector<SubShader>      shaders;
    std::vector<ShaderUniform>  uniforms;
    ContextRef                  context;

    ShaderImpl (ContextRef& context) : context(context) {}

    // Helper functions

    static ShaderRef create (ContextRef context) {
        return std::static_pointer_cast<Shader>(std::make_shared<ShaderImpl>(context));
    }
    SubShader& getShader (GLenum type) {
        for (auto& shader : shaders)
            if (shader.type == type)
                return shader;
        shaders.emplace_back(type, 0, ShaderStatus::EMPTY);
        return shaders.back();
    }
    ShaderUniform& getUniform (UniformType type, const std::string& name) {
        for (auto& uniform : uniforms) {
            if (uniform.type == type && uniform.name == name)
                return uniform;
        }
        switch (type) {
            case UniformType::UNIFORM: 
                uniforms.emplace_back(name, getUniformLocation(handle, name.c_str()), type);
                break;
            case UniformType::SUBROUTINE_NAME:
                assert(0);
                // uniforms.emplace_back(name, getSubroutineUniformLocation(handle, name.c_str()), type);
                break;
            case UniformType::SUBROUTINE_VALUE:
                // ...???
                assert(0);
                break;
        }
        return uniforms.back();
    }
    void markDirtySubshader () {
        uniforms.clear();
    }
    void markDirtyProgram () {
        uniforms.clear();
    }
};
#define self reinterpret_cast<ShaderImpl*>(this)
#define const_self reinterpret_cast<const ShaderImpl*>(this)
#define glstate const_cast<GLState&>(self->context->getState())

const ShaderStatus Shader::status () const { return const_self->status; }

void Shader::attachSource (GLenum type, const char* src, size_t length) {

    // Get current or new shader for this type
    auto& shader = self->getShader(type);
    assert(shader.type == type);

    if (shader.handle) { 
        // Recompiling, so mark uniforms, etc., dirty 
        self->markDirtySubshader();
    }

    // Try compiling shader; throw error if fails
    if (!shader.handle) shader.handle = createShader(type);
    if (!compileShader(shader.handle, type, src, length)) {
        shader.status = self->status = ShaderStatus::COMPILE_ERROR;
        throw GLShaderCompilationException(getShaderInfoLog(shader.handle));
    }

    // Acquire program object + attach shader (should succeed)
    acquireProgram(self->handle);
    attachShader(self->handle, shader.handle);

    // Update status
    self->status  = ShaderStatus::NOT_LINKED;
    shader.status = ShaderStatus::OK;
    
    for (auto& shader : self->shaders) {
        if (shader.status != ShaderStatus::OK) {
            self->status = ShaderStatus::COMPILE_ERROR;
            break;
        }
    }
}

void Shader::relink () {
    // Check shader status
    for (auto& shader : self->shaders) {
        if (shader.status != ShaderStatus::OK) {
            self->status = ShaderStatus::COMPILE_ERROR;
            throw GLShaderLinkException("Program has uncompiled shaders");
        }
    }
    // Try linking program
    acquireProgram(self->handle);
    if (!linkProgram(self->handle)) {
        self->status = ShaderStatus::LINK_ERROR;
        throw GLShaderLinkException(getProgramInfoLog(self->handle));
    }

    // Mark shader ok
    self->status = ShaderStatus::OK;
    self->markDirtyProgram();
}

void Shader::reset () {
    for (auto& shader : self->shaders) {
        deleteShader(shader.handle);
    }
    deleteProgram(self->handle);
    self->shaders.clear();
    self->uniforms.clear();
    self->flags.reset();
    self->status = ShaderStatus::EMPTY;
}

bool Shader::bind () {
    return self->status == ShaderStatus::OK &&
        bindProgram(self->handle, glstate.activeProgram);
}

template <typename T>
void Shader::setUniform (const std::string& name, T value) {
    auto& uniform = self->getUniform(UniformType::UNIFORM, name);
    setUniform(self->handle, uniform.location, value);
}

template <typename T>
void Shader::setUniform (const std::string& name, const T* values, size_t count) {
    auto& uniform = self->getUniform(UniformType::UNIFORM, name);
    setUniform(self->handle, uniform.location, values, count);
}

const std::vector<std::string>& getUniformNames () {
    assert(0); // Unimplemented
}
const std::vector<std::string>& getSubroutineNames () {
    assert(0); // Unimplemented
}
const std::vector<std::string>& getSubroutineValues () {
    assert(0); // Unimplemented
}
void setSubroutine (const std::string& name, const std::string& value) {
    assert(0); // Unimplemented
}

//
// Texture implementation
//

struct TextureImpl : public Texture {
    GLuint      handle;
    ContextRef  context;

    TextureImpl (ContextRef& context) : context(context) {}

    static TextureRef create (ContextRef context) {
        return std::static_pointer_cast<Texture>(std::make_shared<TextureImpl>(context));
    }
};
#undef self
#undef const_self
#define self reinterpret_cast<TextureImpl*>(this)
#define const_self reinterpret_cast<const TextureImpl*>(this)

bool Texture::bind (GLenum target) {
    return bindTexture(target, self->handle, glstate.activeTexture);
}

// TBD...

//
// Buffer implementation
//

struct BufferImpl : public Buffer {
    GLuint      handle;
    ContextRef  context;

    BufferImpl (ContextRef& context) : context(context) {}

    static BufferRef create (ContextRef context) {
        return std::static_pointer_cast<Buffer>(std::make_shared<BufferImpl>(context));
    }
};
#undef self
#undef const_self
#define self reinterpret_cast<BufferImpl*>(this)
#define const_self reinterpret_cast<const BufferImpl*>(this)

bool Buffer::bind (GLenum target) {
    return bindBuffer(target, self->handle, glstate.activeBuffer);
}

//
// VAO implementation
//

struct VertexArrayImpl : public VertexArray {
    GLuint      handle;
    ContextRef  context;

    VertexArrayImpl (ContextRef& context) : context(context) {}

    static VertexArrayRef create (ContextRef context) {
        return std::static_pointer_cast<VertexArray>(std::make_shared<VertexArrayImpl>(context));
    }
};
#undef self
#undef const_self
#define self       reinterpret_cast<VertexArrayImpl*>(this)
#define const_self reinterpret_cast<const VertexArrayImpl*>(this)

bool VertexArray::bind () {
    return bindVertexArray(self->handle, glstate.activeBuffer);
}

//
// Context implementation (manages all of the above + gl state)
//

struct ContextImpl : public Context {
    GLState                     state;
    std::vector<GLResourceRef>  resources;
    std::weak_ptr<Context>      backref;

    ContextImpl () {}

    ContextRef selfref () { return backref.lock(); }
    void addResource (GLResourceRef resource) {
        resources.push_back(std::move(resource));
    }
};
#undef self
#undef const_self
#define self reinterpret_cast<ContextImpl*>(this)
#define const_self reinterpret_cast<const ContextImpl*>(this)

const GLState& Context::getState () const {
    return const_self->state;
}
ShaderRef Context::createShader () {
    auto ref = ShaderImpl::create(self->selfref());
    self->addResource(std::static_pointer_cast<GLResource>(ref));
    return ref;
}
TextureRef Context::createTexture () {
    auto ref = TextureImpl::create(self->selfref());
    self->addResource(std::static_pointer_cast<GLResource>(ref));
    return ref;
}
BufferRef Context::createBuffer () {
    auto ref = BufferImpl::create(self->selfref());
    self->addResource(std::static_pointer_cast<GLResource>(ref));
    return ref;
}
VertexArrayRef Context::createVertexArray () {
    auto ref = VertexArrayImpl::create(self->selfref());
    self->addResource(std::static_pointer_cast<GLResource>(ref));
    return ref;
}
const std::vector<GLResourceRef>& Context::getResources () {
    return self->resources;
}
void Context::purgeInactiveResources () {
    for (size_t i = self->resources.size(), l = i-1; i --> 0; ) {
        if (self->resources[i].use_count() == 1) {
            if (i != l) {
                std::swap(self->resources[i], self->resources[l]);
                --l;
            }
            self->resources.pop_back();
        }
    }
}

ContextRef Context::create () {
    auto context = std::make_shared<ContextImpl>();
    context->backref = std::weak_ptr<Context>(std::static_pointer_cast<Context>(context));
    return std::static_pointer_cast<Context>(context);
}

}; // namespace kgl
