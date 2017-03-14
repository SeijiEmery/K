
#pragma once

namespace kgl {

void glFlushErrors () {
    while (glGetError() != GL_NO_ERROR) {}
}
void glAssertNoError (const char* msg) {
    auto err = glGetError();
    switch (err) {
        case GL_NO_ERROR: break;
        #define ERROR_CASE(e) 
            case e: throw GLRuntimeException(#e, msg); break;
        ERROR_CASE(GL_INVALID_ENUM)
        ERROR_CASE(GL_INVALID_VALUE)
        ERROR_CASE(GL_INVALID_OPERATION)
        ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
        ERROR_CASE(GL_OUT_OF_MEMORY)
        #undef ERROR_CASE
        default: throw GLRuntimeException(format("Unknown error: %s", err), msg); break;
    }
}
#define GL_CHECKED_CALL(fcn, args...) \
    fcn(args); \
    glAssertNoError(#fcn)

namespace resource {
enum class Type {
    NONE = 0, SHADER, VAO, VBO, FBO, count = FBO
};

struct Base {
    virtual Type type () const = 0;
    virtual void create () = 0;
    virtual void release () = 0;
    operator bool () const { return handle != 0; }
    auto get () { if (!*this) create(); return handle; }
protected:
    GLuint handle = 0;
private:
    std::atomic<int> refcnt = 0;

    friend class ResourceRef;
    virtual void retainRef  () { 
        ++refcnt; 
    }
    virtual void releaseRef () {
        auto count = --refcnt;
        assert(count >= 0);
        if (count == 0) {
            KThread::glThread()->send([&this](){
                assert(this); assert(this->refcnt == 0);
                this->release();
                delete *this;
            });
        }
    }
};

struct NullResource : Base {
    static NullResource instance;
    static auto constexpr thisType = Type::NONE;

    Type type    () const override { return thisType; }
    void create  () const override { assert(handle == 0); }
    void release () const override {}
    virtual void retainRef  () override {}
    virtual void releaseRef () override {}
};

struct BaseRef {
    BaseRef (Base* resource)     : resource(resource) { if (resource) resource->retainRef(); }
    BaseRef (BaseRef&& ref)      : resource(std::move(ref.resource)) { ref.resource = nullptr; }
    BaseRef (const BaseRef& ref) : BaseRef(ref.get()) {}

    BaseRef& operator= (const BaseRef& other) {
        if (resource != other.resource) {
            if (resource) resource->releaseRef();
            if (other.resource) other.resource->retainRef();
            resource = other.resource;
        }
        return *this;
    }
    BaseRef& operator= (BaseRef&& other) { return std::swap(resource, other.resource), *this; }

    






    ResourceType type () const { return resource->type(); }
protected:
    Base* resource;
};
class ResourceRef;

template <typename Resource>
struct TRef : public IResource {
    TRef () : resource(nullptr);
    TRef(Resource* resource) : resource(resource) { resource->retainRef(); }
    TRef(const TRef<Resource>& ref) : TRef(ref.get()) {}
    TRef()



    TRef(const ResourceRef& ref);











    Resource* get () const { return resource; }
};












template <typename Resource>
struct TRef {
private:
    void assign (Resource* value) {
        if (resource != value) {
            if (resource) resource->releaseRef();
            if (value)    value->retainRef();
            resource = value;
        }
    }
public:
    TRef () : resource(nullptr) {}
    TRef (Resource* resource) : resource(resource) { resource->retainRef(); }
    TRef (const TRef<Resource>& other) : resource(other.resource) { if (resource) resource->retainRef(); }
    TRef (TRef<Resource>&& other) : resource(std::move(other.resource)) { other.resource = nullptr; }
    TRef& operator= (const TRef<Resource>& other) { return assign(other.get()); }
    TRef& operator= (TRef<Resource>&& other) { return std::swap(resource, other.resource), *this; }
    ~TRef () { resource->releaseRef(); }




    auto type () const { return Resource::thisType; }
    Resource* get () const { return resource; }
private:
    Resource* resource;
};

struct ResourceRef {
private:
    void assign (Base* value) {
        if (resource != value) {
            if (resource) resource->releaseRef();
            if (value)    value->retainRef();
            resource = value;
        }
    }
public:
    ResourceRef () : resource(&NullResource::instance) {}
    template <typename Resource> ResourceRef (Resource* resource) : resource(static_cast<Base*>(resource)) { resource->retainRef(); }
    template <typename Resource> ResourceRef (const TRef<Resource>& ref) : resource(ref.get()) { resource->retainRef(); }
    template <typename Resource> ResourceRef& operator= (const TRef<Resource>& ref) { return assign(ref.get()), *this; }
    ResourceRef (const ResourceRef& other) : resource(other.resource) { resource->retainRef(); }
    ResourceRef (ResourceRef&& other) : resource(std::move(other.resource)) { other.resource = nullptr; }
    ResourceRef& operator= (const ResourceRef& other) { return assign(other.resource), *this; }
    ResourceRef& operator= (ResourceRef&& other) { return std::swap(resource, other.resource), *this; }
    ~ResourceRef () { resource->releaseRef(); }

    #define DEFN_CMP(op) \
    friend bool operator op (const ResourceRef& a, const ResourceRef& b) const { return a.resource op b.resource; } \



    template <typename R>
    friend bool operator op (const ResourceRef& a, const TRef<R>& b) const {


    } \
    DEFN_CMP(==)
    DEFN_CMP(!=)
    DEFN_CMP(>=)
    DEFN_CMP(<=)
    DEFN_CMP(>)
    DEFN_CMP(<)
    #undef DEFN_CMP

    auto type () const { return resource->type(); }
    template <typename Resource>
    Resource* get () const {
        return type() == Resource::thisType ?
            static_cast<Resource*>(resource) :
            nullptr;
    }
    template <typename Resource, typename F>
    bool exec (F f) {
        if (type() == Resource::thisType) {
            KThread::glThread()->send([resource,f](){
                f(resource);
            });
            return true;
        }
        return false;
    }
private:
    Base* resource;
};








struct VBO : public Base {
    enum class Buffering {
        STATIC_DRAW = GL_STATIC_DRAW, DYNAMIC_DRAW = GL_DYNAMIC_DRAW
    };
    static auto constexpr thisType = Type::VBO;
    Type type () const override { return thisType; }

    // Public methods
    void create () override {
        if (handle == 0) {
            GL_CHECKED_CALL(glGenBuffers, 1, &handle);
            assert(handle != 0);
            GL_TRACE_CREATED(thisType, handle);
        }
    }
    void release () override {
        if (handle != 0) {
            GL_TRACE_RELEASED(thisType, handle);
            GL_CHECKED_CALL(glDeleteBuffers, 1, &handle);
            handle = 0;
        }
    }
    void bind (VBO*& prev) {
        if (this != prev) {
            prev = this;
            GL_TRACE_BINDING_RESOURCE(thisType, handle);
            GL_CHECKED_CALL(glBindBuffer, GL_ARRAY_BUFFER, handle);
        }
    }
    void bufferData (const void* data, size_t size, Buffering buffering) {
        GL_CHECKED_CALL(glBufferData, GL_ARRAY_BUFFER, size, data, (GLuint)buffering);
    }
};

struct VAO : public Base {
    static auto constexpr thisType = Type::VAO;
    Type type () const override { return thisType; }

    // Public methods
    void create () override {
        if (handle == 0) {
            GL_CHECKED_CALL(glGenVertexArrays, 1, &handle);
            assert(handle != 0);
            GL_TRACE_CREATED(thisType, handle);
        }
    }
    void release () override {
        if (handle != 0) {
            GL_TRACE_RELEASED(thisType, handle);
            GL_CHECKED_CALL(glDeleteVertexArrays, 1, &handle);
            handle = 0;
        }
    }
    // Usage:
    //  1. <some_vao>.bind(current_vao);
    //  2. <some_vbo>.bind(current_vbo);
    //  3. <
    //
    void bindVertexAttrib (unsigned index, unsigned count, unsigned glDataType, GLboolean normalized, int stride, void* offset) {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, count, glDataType, normalized, stride, offset);
    }
};

class Context {
    // Bound objects
    Shader* boundShader = nullptr;
    VAO*    boundVao    = nullptr;
    VBO*    boundVbo    = nullptr;
private:
    void doBindShader (GLuint handle) {
        GL_TRACE_BINDING_RESOURCE(Type::SHADER, handle);
        GL_CHECKED_CALL(glUseProgram, handle);
    }
    void doBindVao (GLuint handle) {
        GL_TRACE_BINDING_RESOURCE(Type::VAO, handle);
        GL_CHECKED_CALL(glBindVertexArray, handle);
    }
    void doBindVbo (GLuint handle) {
        GL_TRACE_BINDING_RESOURCE(Type::VBO, handle);
        GL_CHECKED_CALL(glBindBuffer, handle);   
    }
private:
    // public methods
    void bind (Shader* value) {
        if (boundShader != value) {
            boundShader = value;
            doBindShader(value ? value->get() : 0);
        }
    }
    void bind (VAO* value) {
        if (boundVao != value) {
            boundVao = value;
            doBindVao(value ? value->get() : 0);
        }
    }
    void bind (VBO* value) {
        if (boundVbo != value) {
            boundVbo = value;
            doBindVbo(value ? value->get() : 0);
        }
    }

    // VBO operations

    template <typename Container>
    void bufferData (VBO& vbo, const Container& container, VBO::Buffering buffering) {
        bufferData(vbo, container.data(), container.size() * sizeof(container[0]), buffering);
    }
    void bufferData (VBO& vbo, const void* data, size_t size, VBO::Buffering buffering) {
        bind(&vbo);
        GL_CHECKED_CALL(glBufferData,GL_ARRAY_BUFFER, size, data, (unsigned)buffering);
    }

    // VAO operations
};
}; // namespace resource

namespace command {

struct BufferData {
    resource::ResourceRef 
};



};










}













struct MeshData {
    KMemoryPool                 data;
    KRange<vec3>                verts;
    KRange<vec3>                normals;
    KRange<vec2>                uvs;
    KVariantRange<int, short>   indices;
};

// Simple example...

struct StaticMeshComponent {
    KManaged<MeshData>  mesh;
    KManaged<Transform> transform;

};








struct Mesh {

};




















