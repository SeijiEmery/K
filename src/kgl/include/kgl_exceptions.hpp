
#pragma once
#include <exception>

#define DEFN_EXCEPTION(Ex,Base) \
struct Ex : public Base {\
    Ex (const std::string& what) : Base(what) {}\
};

DEFN_EXCEPTION(GLException, std::runtime_error)
DEFN_EXCEPTION(GLShaderCompilationException, GLException)
DEFN_EXCEPTION(GLShaderLinkException, GLException)

#undef DEFN_EXCEPTION
