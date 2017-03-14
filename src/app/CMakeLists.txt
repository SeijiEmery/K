
project(k::app VERSION 0.1 LANGUAGES CXX)

include_directories(include ${EXT_GLFW_INCLUDE})
add_executable(app src/main.cpp)
target_link_libraries(app glfw)

# find_package(GLM REQUIRED)
# include_directories(${GLM_INCLUDE_DIRS})

# add_library(kapp
#     src/app_instance.cxx
#     src/app_device_manager.cxx
#     src/app_event_manager.cxx
#     src/app_thread_manager.cxx
#     src/app_window_manager.cxx
#     src/backend_glfw_app.cxx
#     src/backend_threads.cxx
# )

# target_include_directories(kapp PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#     $<INSTALL_INTERFACE:include>
#     PRIVATE src
# )

# target_compile_features(kapp
#     PUBLIC
#     PRIVATE
# )

# export(TARGETS kapp FILE KAppLibConfig.cmake)
