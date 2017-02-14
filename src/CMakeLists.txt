
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(kproject VERSION 0.1 LANGUAGES CXX)

find_package(Boost REQUIRED)
add_library(boost INTERFACE IMPORTED)
set_property(TARGET boost PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_subdirectory(app ../build/app)
add_subdirectory(../demos/window_test ../build/window_test)
