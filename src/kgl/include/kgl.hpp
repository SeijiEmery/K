//
// Yet another high-level OpenGL wrapper. (targets OpenGL 4.1 / GLFW3)
//

#pragma once
#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <memory>
#include "kgl_exceptions.hpp"

namespace kgl {

//
// OpenGL objects. 
// Context is an object manager and has some internal state for managing bindings, etc.
//

class Context; typedef std::shared_ptr<Context> ContextRef;
class Shader;  typedef std::shared_ptr<Shader>  ShaderRef;
class Texture; typedef std::shared_ptr<Texture> TextureRef;
class Buffer;  typedef std::shared_ptr<Buffer>  BufferRef;
class VertexArray; typedef std::shared_ptr<VertexArray> VertexArrayRef;

//
// Root resource type; all of the above inherit from this.
// Use .type() / ResourceType to determine the type of a given object.
//

enum class ResourceType { Context, Shader, Texture, Buffer, VertexArray };
class GLResource {
public:
    virtual ResourceType type () = 0;
    virtual ~GLResource () {}
};
typedef std::shared_ptr<GLResource> GLResourceRef;

//
// Useful macros. Undefined at the end of this file.
//

// Disables copy ctors; uses move ctors instead.
#define DISABLE_COPY_CTORS(T)\
    T (const T&) = delete;\
    T (T&&) = default;\
    T& operator= (const T&) = delete;\
    T& operator= (T&&) = default;

// Implements memory semantics for our resource types:
// – can only be constructed by Context, and managed *exclusively* through shared_ptrs.
// – copy ctors disabled; move ctors enabled (for std::shared_ptr).
// – also implements our virtual type() method.
#define DECL_RESOURCE_CTORS(T)\
protected:\
    T () {}\
    friend class Context;\
public:\
    DISABLE_COPY_CTORS(T)\
    ~T() {}\
    ResourceType type () override { return ResourceType::T; }

//
// Public interfaces. All of the following classes are final.
// (we're doing some clever things to hide our implementation + all member variables).
//

// Possible shader states. Shader may be bound iff status() == ShaderStatus::OK.
enum class ShaderStatus { 
    OK=0,               // Ok. Program is fully compiled + linked.
    EMPTY,              // Initial state (not bindable). Set to this after reset().
    NOT_LINKED,         // Has called attachSource() but not relink()
    COMPILE_ERROR,      // Error compiling one or more sub-shaders (from attachSource)
    LINK_ERROR          // Error linking program, outside of compile errors (from relink)
};

// Abstracts an opengl shader program. Create using Context.
class Shader : public GLResource {
    DECL_RESOURCE_CTORS(Shader)

    // Get current shader status
    const ShaderStatus status () const;

    // Compile + attach shader from source; throws ShaderCompilationException
    void attachSource (GLenum type, const char* src, size_t length);

    // Link program; throws ShaderLinkException
    void relink ();

    // Unbind + delete all attached shaders; resets state to ShaderStatus::EMPTY.
    void reset ();

    // Try binding program; returns true iff successful (program can be bound + is bound)
    bool bind ();

    // Set shader uniform
    template <typename T>
    void setUniform (const std::string& name, T value);

    // Set shader uniform array
    template <typename T>
    void setUniform (const std::string& name, const T* values, size_t count);

    // Introspection: get list of shader uniforms
    const std::vector<std::string>& getUniformNames ();

    // Introspection: get list of shader subroutines
    const std::vector<std::string>& getSubroutineNames ();

    // Introspection: get list of values for a given subroutine
    const std::vector<std::string>& getSubroutineValues ();

    // Set shader subroutine. Throws InvalidShaderSubroutineException.
    void setSubroutine (const std::string& name, const std::string& value);
};


// Abstracts an opengl texture. Create using Context.
class Texture : public GLResource {
    DECL_RESOURCE_CTORS(Texture)

    // Try binding texture; returns true iff successful.
    bool bind (GLenum target);

    // TBD...
};

// Abstracts an opengl buffer object. Create using Context.
class Buffer : public GLResource {
    DECL_RESOURCE_CTORS(Buffer)

    // Try binding buffer; returns true iff successful.
    bool bind (GLenum target);

    // TBD...
};

// Abstracts an opengl vertex array. Create using Context.
class VertexArray : public GLResource {
    DECL_RESOURCE_CTORS(VertexArray)

    bool bind ();

    // TBD...
};

// Stores current opengl state. Will be expanded to include scissoring, etc.
struct GLState {
    GLuint activeProgram = 0;
    GLuint activeTexture = 0;
    GLuint activeBuffer  = 0;
    GLuint activeVertexArray = 0;
};

// Manages other resources + GLState.
// Keeps references to all created resources so we can control object destruction.
// (ie. run dtors on a specific thread).
//
// This class's dtor and purgeInactiveResources() must be called from your active GL thread.
//
class Context : public GLResource {
    DECL_RESOURCE_CTORS(Context)

    // Create a context instance.
    static ContextRef create ();

    // Get readonly access to current GL state.
    const GLState&  getState () const;

    // Create resources
    ShaderRef       createShader  ();
    TextureRef      createTexture ();
    BufferRef       createBuffer  ();
    VertexArrayRef  createVertexArray ();

    // Get list of all active / non-active resources
    // Resource is inactive if use_count == 1.
    const std::vector<GLResourceRef>& getResources ();

    // Release all inactive resources.
    void purgeInactiveResources ();
};

}; // namsepace kgl
