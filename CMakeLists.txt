cmake_minimum_required (VERSION 3.2 FATAL_ERROR)
project(KSandbox)

set(CMAKE_CXX_STANDARD 14)

# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw" "../ext_build/glfw")
# set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/assimp"       "../ext_build/assimp")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/die-xml"      "../ext_build/die-xml")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/googletest"   "../ext_build/googletest")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/haiyai"       "../ext_build/haiyai")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui"      "../ext_build/nanogui")

set(EXT_ASSIMP_INCLUDE             "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/include")
set(EXT_CONCURRENT_QUEUE_INCLUDE   "${CMAKE_CURRENT_SOURCE_DIR}/ext/concurrentqueue/include")
set(EXT_DIE_XML_INCLUDE            "${CMAKE_CURRENT_SOURCE_DIR}/ext/die-xml/include")
set(EXT_EXPECTED_INCLUDE           "${CMAKE_CURRENT_SOURCE_DIR}/ext/expected/include")
set(EXT_GLM_INCLUDE                "${CMAKE_CURRENT_SOURCE_DIR}/ext/glm/include")
set(EXT_GTEST_INCLUDE              "${CMAKE_CURRENT_SOURCE_DIR}/ext/googletest/include")
set(EXT_HAYAI_INCLUDE              "${CMAKE_CURRENT_SOURCE_DIR}/ext/haiyai/include")
set(EXT_NANOGUI_INCLUDE            "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include")
set(EXT_NANOVG_INCLUDE             "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/nanovg/include")
set(EXT_GLFW_INCLUDE               "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/glfw/include")
set(EXT_EIGEN_INCLUDE              "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/eigen/include")

add_subdirectory(src)
