
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(WindowTest VERSION 0.1 LANGUAGES CXX)

add_executable(window_test
    src/window_test.cxx)
target_link_libraries(window_test
    kapp)
