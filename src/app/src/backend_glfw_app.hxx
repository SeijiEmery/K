
#pragma once

#include "app_window_manager.hxx"
#include <GLFW/glfw3.hpp>
#include "types.hxx"

namespace k {
namespace app {
namespace backend {

struct WindowProperties {
    std::string name;
    std::string title;
    std::ivec2  size;
    std::ivec2  framebufferSize;
    double      scaleFactor = 0;
    bool        active;
};

class Window {
    std::unique_ptr<GLFWwindow> window;
    WindowProperties            properties;
};

struct WindowCommand {
    std::weak_ptr<Window> window;
    virtual void execute () {}
};

namespace Command {

struct CreateWindow : public WindowCommand {
    CreateWindow (decltype(window) window) : window(window) {}
    void execute () override;
};
struct DestroyWindow : public WindowCommand {
    DestroyWindow (decltype(window) window) : window(window) {}
    void execute () override;
};
struct SetWindowName : public WindowCommand {
    std::string name;
    SetWindowName (decltype(window) window, decltype(name) name) 
        : window(window), name(name) {}
    void execute () override;
};
struct SetWindowTitle : public WindowCommand {
    std::string title;
    SetWindowTitle (decltype(window) window, delctype(title) title)
        : window(window), title(title) {}
    void execute () override;
};
struct SetWindowSize : public WindowCommand {
    std::ivec2 windowSize;
    std::ivec2 framebufferSize;
    SetWindowSize (decltype(window) window, decltype(windowSize) windowSize, decltype(framebufferSize) framebufferSize)
        : window(window), windowSize(windowSize), framebufferSize(framebufferSize) {}
    void execute () override;
};
struct SetWindowActive : public WindowCommand {
    bool active;
    SetWindowActive (decltype(window) window, decltype(active) active)
        : window(window), active(active) {}
    void execute () override;
};

}; // namespace Command










class Application {

};













};
};
};
