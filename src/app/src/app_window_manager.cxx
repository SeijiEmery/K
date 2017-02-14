
#include "app_window_manager.hxx"
#include "backend_app_facade.hxx"

using namespace k::app;

// FACADE CLASSES

struct WindowImpl {
    backend::WindowManager  windowManager;
    backend::WindowRef      window;
    backend::WindowProperties properties;

    WindowImpl (void* appContext) :
        windowManager(static_cast<backend::AppFacadeBridge*>->windowManager),
        window(nullptr),
        properties(static_cast<backend::AppFacadeBridge*>->defaultWindowProperties)
    {}

    void create () {
        if (!window) {
            window = windowManager->createAndLinkWindow(&properties, &window);
        }
    }
    void destroy () {
        if (window) {
            windowManager->destroyWindow(window);
            window = nullptr;
        }
    }

    #define DEFN_PROPERTY(name, T, Cls, Command) \
    Cls name; \
    struct Cls : public IProperty<T> { \
        WindowImpl* window; \
        Cls (WindowImpl* window) : window(window) {} \
        bool get () override { return properties->name; } \
        void set (T v) override { \
            properties->name = v; \
            if (window) window->send<Command>(v, &(properties->name)); \
        } \
    }; \

    DEFN_PROPERTY(active, bool,         ActiveProperty, Command::SetWindowActive);
    DEFN_PROPERTY(name,   std::string,  NameProperty,   Command::SetWindowName);
    DEFN_PROPERTY(active, std::string,  TitleProperty,  Command::SetWindowTitle);
    DEFN_PROPERTY(size,   glm::ivec2,   SizeProperty,   Command::SetWindowSize);
    DEFN_PROPERTY(screen, Screen,       ScreenProperty, Command::SetWindowScreen);
    DEFN_PROPERTY(pixelScaleFactor, Window::PixelScaleFactor, PSFProperty, Command::SetWindowPixelScale);

    #undef DEFN_PROPERTY

    // #define DEFN_PROPERTY(name, T, Cls) \
    // Cls name; \
    // struct Cls : public IProperty<T> { \
        
    // #define END_PROPERTY };

    // DEFN_PROPERTY(active, bool, ActiveProperty)
    //     bool get () override { return properties->isActive; }
    //     void set (bool active) {
    //         properties->isActive = active;
    //         if (window) window->send<Command::SetWindowActive>(active);
    //     }
    // END_PROPERTY
};

Window::Window (void* applicationContext)
    : impl(new WindowImpl(applicationContext)),
      active(&(impl.active)),
      name(&(impl.name),
      title(&(impl.title)),
      size(&(impl.size)),
      screen(&(impl.screen)),
      pixelScaleFactor(&(implpixelScaleFactor))
{}
void Window::create () { impl->create(); }
void Window::destroy () { impl->destroy(); }



class WindowManagerImpl {
    // TBD
};

WindowManager::WindowManager (void* applicationContext)
    : impl(new WindowManagerImpl(applicationContext))
{}

Window& WindowManager::operator[] (const std::string& name) {
    return impl->getWindow(name);
}
const std::vector<std::string>& WindowManager::windows () {
    return impl->getWindowList();
}
