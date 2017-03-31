
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(kgl)

find_package(OpenGL REQUIRED)

include_directories("include" ${EXT_GLFW_INCLUDE} ${OPENGL_INCLUDE_DIR})
add_library(kgl "src/kgl.cpp")
target_link_libraries(kgl glfw glew ${OPENGL_gl_LIBRARY})

