
#include "backend_glfw_app.hxx"

namespace k {
namespace app {
namespace backend {
namespace Command {

// Creates a window w/ the current window settings (window properties).
// If window was not created, creates the window + sends a WindowCreated event.
// If window was already created, does nothing.
void CreateWindow::execute () {
    if (auto w = window.lock()) {
        if (!w->window) {
            glfwWindowHint(GLFW_RESIZABLE,  w->windowCreationInfo.resizable);
            glfwWindowHint(GLFW_RESIZABLE,  w->windowCreationInfo.visible);
            glfwWindowHint(GLFW_RESIZABLE,  w->windowCreationInfo.decorated);

            glfwWindowHint(GLFW_RED_BITS,   w->windowCreationInfo.colorDepth.r);
            glfwWindowHint(GLFW_GREEN_BITS, w->windowCreationInfo.colorDepth.g);
            glfwWindowHint(GLFW_BLUE_BITS,  w->windowCreationInfo.colorDepth.b);
            glfwWindowHint(GLFW_ALPHA_BITS, w->windowCreationInfo.colorDepth.a);

            glfwWindowHint(GLFW_DEPTH_BITS, w->windowCreationInfo.depthBits);
            glfwWindowHint(GLFW_STENCIL_BITS, w->windowCreationInfo.stencilBits);

            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

            int major, int minor; bool coreProfile;
            switch (w->app->config.opengl_version) {
                case OpenGLVersion::v21: major = 2, minor = 1, coreProfile = false; break;
                case OpenGLVersion::v32: major = 3, minor = 2, coreProfile = true; break;
                case OpenGLVersion::v41: major = 4, minor = 1; coreProfile = true; break;
                case OpenGLVersion::v45: major = 4, minor = 5; coreProfile = true; break;
                default: assert("Invalid opengl version!");
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, !coreProfile);
            glfwWindowHint(GLFW_OPENGL_PROFILE, coreProfile ? GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_COMPAT_PROFILE);

            // I think that's all of the relevant options -- see http://www.glfw.org/docs/3.0/window.html#window_hints

            w->window = glfwCreateWindow(
                w->properties.size.x, w->properties.size.y,     // window dimensions
                w->properties.title.c_str(),                    // window title
                w->getGLFWMonitorFromProperties(),              // monitor
                w->getPrimaryActiveMonitor(),                   // get active monitor (to share resources)
            );
            if (w->window == nullptr) {
                throw new GlfwException("Could not create window '" + w->name + "'");
            }

            glfwSetWindowUserPointer(w->window, w);

            w->updateWindowSizeInfo();

            glfwSetWindowCloseCallback(w->window, onWindowCloseCallback);
            glfwSetWindowSizeCallback(w->window, onWindowSizeCallback);
            glfwSetFramebufferSizeCallback(w->window, onFramebufferSizeCallback);
            glfwSetWindowPosCallback(w->window, onWindowPosCallback);

            glfwSetKeyCallback(w->window, onKeyCallback);
            glfwSetCharCallback(w->window, onCharCallback);
            glfwSetMouseButtonCallback(w->window, onMouseButtonCallback);
            glfwSetCursorPosCallabck(w->window, onCursorPosCallback);
            glfwSetCursorEnterCallback(w->window, onCursorEnterCallback);
            glfwSetScrollCallback(w->window, onScrollCallback);

            w->app->notify<Event::WindowCreated>(window);
        }
    }
}

// Destroy a window.
// If window exists, dispatches a WindowDestroyed event before destroying the window.
// If window does not exist, does nothing.
void DestroyWindow::execute () {
    if (auto w = window.lock()) {
        if (w->window) {
            w->app->notify<Event::WindowDestroyed>(window);
            glfwDestroyWindow(w->window);
            w->window = nullptr;
        }
    }   
}

// Set a window's global name (renames that window).
// If name changes, dispatches a WindowRenamed event w/ the prev + new window name.
void SetWindowName::execute () {
    if (auto w = window.lock()) {
        if (w->properties.name != name) {
            w->app->notify<Event::WindowRenamed>(window, w->properties.name, name);
            w->app->renameWindow(w, name);
        }
    }
}

// Set a window to hidden / unhidden.
// If state changes, dispatches a WindowVisibilityChanged event w/ prev + current visibility state.
void SetWindowActive::execute () {
    if (auto w = window.lock()) {
        bool isActive = !glfwGetWindowAttrib(w->window, GLFW_HIDDEN);
        if (isActive != active) {
            w->app->notify<Event::WindowVisibilityChanged>(window, isActive, active);
            if (active)     glfwShowWindow(w->window);
            else            glfwHideWindow(w->window);
            assert(glfwGetWindowAttrib(window, GLFW_HIDDEN) != active);
        }
    }
}


// Give this window input focus.
// If not focused, dispatches a WindowFocusChanged event w/ the prev + current foucsed windows.
void SetWindowFocus::execute () {
    if (auto w = window.lock()) {
        auto isFocused = glfwGetWindowAttrib(w->window, GLFW_FOCUSED);
        if (!isFocused) {
            auto focusedWindow = w->app->getCurrentlyFocusedWindow();
            w->app->notify<Event::WindowFocusChanged>(window, focusedWindow, w);
            glfwFocusWindow(w->window);
        }
    }
}

// Sets a window to minimized / maximized.
// Does not fire any events directly, though this should trigger a window hidden / not event (I think...?)
void SetWindowMinimized::execute () {
    if (auto w = window.lock()) {
        auto isMinimized = glfwGetWindowAttrib(w->window, GLFW_ICONIFIED);
        if (minimize != isMinimized) {
            // Don't send event -- should get caught by event callbacks, plus who
            // cares if window is minimized or not...
            if (minimize)   glfwIconifyWindow(w->window);
            else            glfwMaximizeWindow(w->window);
        }
    }
}

void SetWindowSize::execute () {
    if (auto w = window.lock()) {
        if (windowSize != w->properties.windowSize) {
            glfwSetWindowSize(w->window, windowSize.x, windowSize.y);
        }
        if (framebufferSize != w->properties.framebufferSize) {
            glfwSetFramebufferSize(w->window, framebufferSize.x, framebufferSize.y);
        }
    }
}

void SetWindowPos::execute () {
    if (auto w = window.lock()) {
        if (windowPos != w->properties.position) {
            glfwSetWindowPos(w->window, windowPos.x, windowPos.y);
        }
    }
}

void SetWindowTitle::execute () {
    if (auto w = window.lock()) {
        if (title != w->properties.title) {
            w->app->notify<Event::WindowTitleChanged>(window, w->properties.title, title);
            glfwSetWindowTitle(w->window, title.c_str());
            w->properties.title = title;
        }
    }
}

} // namespace Command

}
}
}
