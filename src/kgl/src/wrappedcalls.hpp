
#pragma once
#include <cassert>
#include <glm/glm.hpp>

//
// Error checking, etc.
//

template <typename F>
void glCheckError (const F& callback, const std::string& msg, size_t line, const char* file) {
    auto err = glGetError();
    if (err) {
        constexpr size_t BUFSZ = 4096;
        static char buf[BUFSZ];
        snprintf(&buf[0], BUFSZ, "%s:%lu\t%s",file,line,msg.c_str());
        callback(&buf[0]);
    }
}
template <typename Ex>
void glEnforceOk (const std::string& msg, size_t line, const char* file) {
    glCheckError([](const char* err){
        throw Ex(std::string(err));
    }, msg, line, file);
}
template <typename Log>
void glLogError (Log& log, const std::string& msg, size_t line, const char* file) {
    glCheckError([&log](const char* err){
        log << err << '\n';
    }, msg, line, file);
}
#define GL_ENFORCE_MSG(Ex,msg) glEnforceOk<Ex>(msg,__LINE__,__FILE__)
#define GL_ENFORCE_EXPR(Ex,expr,msg) do {\
    (expr); \
    glEnforceOk<Ex>(msg,__LINE__,__FILE__); \
} while(0)
#define GL_ENFORCE_CALL(fcn,args...) \
    GL_ENFORCE_EXPR(GLException,fcn(args),"Failed call: " #fcn)
#define GL_ENFORCE_CALLR(fcn,rv,args...) \
    GL_ENFORCE_EXPR(GLException,(rv=fcn(args)),"Failed call: " #fcn)
#define GL_ENFORCE_CALLR0(fcn,rv) \
    GL_ENFORCE_EXPR(GLException,(rv=fcn()),"Failed call: " #fcn)


//
// Common calls for GL objects (shader, texture, etc):
// 
/*

/// Lazily creates XXX: calls glCreateXXX / glGenXXX iff x == 0; returns x.
GLuint acquireXXX (GLuint& x);

/// Lazily binds XXX: if x != prev, calls glBindXXX and sets prev = x.
/// Returns true iff x bound (x != 0).
bool bindXXX (GLuint& x, GLuint& prev);

/// Deletes XXX if x != 0; sets x = 0; returns x.
GLuint deleteXXX (GLuint& x);

*/

#define DEFN_ACQUIRE_CALL(Name,handle,CREATE)\
GLuint acquire##Name (GLuint& handle) {\
    if (!handle) {\
        CREATE;\
    }\
    return handle;\
}
#define DEFN_DELETE_CALL(Name,handle,DELETE)\
GLuint delete##Name (GLuint& handle) {\
    if (handle) {\
        DELETE;\
        handle = 0;\
    }\
    return handle;\
}
#define DEFN_BIND_CALL(Name,handle,BIND)\
GLuint bind##Name (GLuint& handle, GLuint& prev) {\
    if (handle != prev) {\
        handle = prev;\
        BIND;\
    }\
    return handle != 0;\
}

//
// Shader Program
//

DEFN_ACQUIRE_CALL(Program,program,GL_ENFORCE_CALLR0(glCreateProgram,program))
DEFN_DELETE_CALL(Program,program,GL_ENFORCE_CALL(glDeleteProgram,program))
DEFN_BIND_CALL(Program,program,GL_ENFORCE_CALL(glUseProgram,program))

//
// Shader
//

GLuint createShader (GLenum shaderType) {
    GLuint shader;
    GL_ENFORCE_CALLR(glCreateShader, shader, shaderType);
    return shader;
}
DEFN_DELETE_CALL(Shader,shader,GL_ENFORCE_CALL(glDeleteShader,shader))

// Compile + attach shader

bool compileShader (GLuint& shader, GLenum type, const char* src, size_t len) {
    assert(shader != 0);

    GLint length = len;
    GL_ENFORCE_CALL(glShaderSource,  shader, 1, &src, &length);
    GL_ENFORCE_CALL(glCompileShader, shader);

    int result = GL_FALSE;
    GL_ENFORCE_CALL(glGetShaderiv, shader, GL_COMPILE_STATUS, &result);
    return result == GL_TRUE;
}
std::string getShaderInfoLog (GLuint shader) {
    assert(shader != 0);
    int length = 0;
    GL_ENFORCE_CALL(glGetShaderiv, shader, GL_INFO_LOG_LENGTH, &length);
    char msg[length+1];
    GL_ENFORCE_CALL(glGetShaderInfoLog, shader, length, &length, &msg[0]);
    return std::string(msg);
}
void attachShader (GLuint& program, const GLuint& shader) {
    assert(shader != 0 && program != 0);    
    GL_ENFORCE_CALL(glAttachShader, program, shader);
}

// Link program

bool linkProgram (const GLuint& program) {
    assert(program != 0);
    GL_ENFORCE_CALL(glLinkProgram, program);

    int result = GL_FALSE;
    GL_ENFORCE_CALL(glGetProgramiv, program, GL_LINK_STATUS, &result);
    return result == GL_TRUE;
}
std::string getProgramInfoLog (GLuint program) {
    assert(program != 0);
    GLint length;
    GL_ENFORCE_CALL(glGetProgramiv, program, GL_INFO_LOG_LENGTH, &length);
    char msg[length+1];
    GL_ENFORCE_CALL(glGetProgramInfoLog, program, length, &length, &msg[0]);
    return std::string(msg);
}

// Fetch uniform locations

GLint getUniformLocation (const GLuint& program, const char* name) {
    GLint location = -1;
    if (program) {
        GL_ENFORCE_CALLR(glGetUniformLocation, location, program, name);
    }
    return location;
}

GLint getSubroutineUniformLocation (GLuint program, GLenum shaderType, const char* name) {
    GLint location = -1;
    if (program) {
        GL_ENFORCE_CALLR(glGetSubroutineUniformLocation, location, program, shaderType, name);
    }
    return location;
}

// Get / set uniform values

#define DEFN_UNIFORM_ELEMENT(T,SET)\
void setUniform (GLuint program, GLint location, T value) {\
    GLint loc;\
    if (program && location != -1) {\
        SET;\
    }\
}
#define DEFN_UNIFORM_ARRAY(T,SET)\
void setUniform (GLuint program, GLint location, const T* values, size_t count){\
    GLuint loc;\
    if (program && location != -1) {\
        SET;\
    }\
}
#define DEFN_UNIFORM_T(T,set,setv) \
    DEFN_UNIFORM_ELEMENT(T,GL_ENFORCE_CALL(set,loc,value))\
    DEFN_UNIFORM_ARRAY(T,GL_ENFORCE_CALL(setv,loc,count,values))

#define DEFN_UNIFORM_V(T,setv)\
    DEFN_UNIFORM_ELEMENT(T,GL_ENFORCE_CALL(setv,loc,1,&(value[0])))\
    DEFN_UNIFORM_ARRAY(T,GL_ENFORCE_CALL(setv,loc,count,&(values[0][0])))

#define DEFN_UNIFORM_M(T,setv,tr)\
    DEFN_UNIFORM_ELEMENT(T,GL_ENFORCE_CALL(setv,loc,1,tr,&(value[0][0])))\
    DEFN_UNIFORM_ARRAY(T,GL_ENFORCE_CALL(setv,loc,count,tr,&(values[0][0][0])))


DEFN_UNIFORM_T(int,      glUniform1i,  glUniform1iv)
DEFN_UNIFORM_T(float,    glUniform1f,  glUniform1fv)

DEFN_UNIFORM_V(glm::vec2, glUniform2fv)
DEFN_UNIFORM_V(glm::vec3, glUniform3fv)
DEFN_UNIFORM_V(glm::vec4, glUniform4fv)

DEFN_UNIFORM_M(glm::mat2, glUniformMatrix2fv, false)
DEFN_UNIFORM_M(glm::mat3, glUniformMatrix3fv, false)
DEFN_UNIFORM_M(glm::mat4, glUniformMatrix4fv, false)


#undef DEFN_UNIFORM_ELEMENT
#undef DEFN_UNIFORM_ARRAY
#undef DEFN_UNIFORM_T
#undef DEFN_UNIFORM_V
#undef DEFN_UNIFORM_M


//
// Texture
//

DEFN_ACQUIRE_CALL(Texture,texture,GL_ENFORCE_CALL(glGenTextures,1,&texture))
DEFN_DELETE_CALL(Texture,texture,GL_ENFORCE_CALL(glDeleteTextures,1,&texture))

GLuint bindTexture (GLenum target, GLuint& texture, GLuint& prev) {
    if (texture != prev) {
        prev = texture;
        GL_ENFORCE_CALL(glBindTexture,target,texture);
    }
    return texture != 0;
}

//
// Buffer
//

GLuint bindBuffer (GLenum target, GLuint& buffer, GLuint& prev) {
    if (buffer != prev) {
        prev = buffer;
        GL_ENFORCE_CALL(glBindBuffer,target,buffer);
    }
    return buffer != 0;
}

//
// Vertex Array
//

DEFN_BIND_CALL(VertexArray,vao,GL_ENFORCE_CALL(glBindVertexArray,vao))


//
// etc...
//

#undef DEFN_ACQUIRE_CALL
#undef DEFN_DELETE_CALL
#undef DEFN_BIND_CALL





